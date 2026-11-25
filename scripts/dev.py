#!/usr/bin/env python3
"""The single entry point for building and verifying the Nexum.

Usage: py scripts/dev.py [--compiler gcc] <command>  (use python3 on Linux and macOS)

  verify     everything: format, build, tests, sanitizers, player build, static analysis and checks
  quick      format, build, tests and checks (runs before each commit)
  configure  prepares the build folder and the third-party libraries
  build      compiles the engine, the kit and the MU module
  test       runs the automated tests
  sanitize   compiles and runs the tests with the memory sanitizers (Windows and Linux with Clang)
  release    compiles and tests the player build, without the authoring mode
  format     checks the C++ formatting (--fix corrects it)
  tidy       runs the static analysis
  checks     runs the repository checks in scripts/checks
  pytests    runs the tests of the checks and of the hooks
  vcpkg-baseline  prints the vcpkg version pinned by this repository

--compiler gcc uses GCC instead of Clang on Linux.

Outside the CI, each command's output goes to build/dev-last.log and only its numbers are shown: tests passed, build
steps, lines with warnings. A command that fails is shown in full. --full shows everything, as the CI always does.

This file is a protected "trava" (see AGENTS.md).
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from collections.abc import Callable, Sequence
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = platform.system() == "Windows"
TIDY_FOLDERS = ("Engine", "Kit", "Games", "Tools")


def system_name(platform_name: str) -> str:
    """The first word of the preset names: windows, linux or macos."""
    return {"Windows": "windows", "Darwin": "macos"}.get(platform_name, "linux")


SYSTEM = system_name(platform.system())


class StepFailed(Exception):
    """A verification step did not pass."""


def preset_name(system: str, compiler: str, kind: str) -> str:
    """The CMake preset for this system, compiler and kind of build (dev, asan or release)."""
    if compiler == "gcc":
        if system != "linux" or kind != "dev":
            raise StepFailed("GCC is used only on Linux, for the dev build (CMakePresets.json).")
        return "linux-gcc-dev"
    return f"{system}-{kind}"


def has_sanitizers(system: str, compiler: str) -> bool:
    """The sanitizers run with MSVC on Windows and with Clang on Linux (docs/adr/0014-*.md)."""
    return compiler == "default" and system in ("windows", "linux")


def without_git_variables(environment: dict[str, str]) -> dict[str, str]:
    """Git hooks export variables that would redirect the Git commands of the tests to this repository."""
    return {name: value for name, value in environment.items() if not name.upper().startswith("GIT_")}


def visual_studio_path() -> Path:
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        raise StepFailed("Visual Studio was not found. Install it with the C++ tools (docs/setup/DEV.md).")
    output = subprocess.run(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        capture_output=True, text=True, check=False,
    ).stdout.strip()
    if not output:
        raise StepFailed("Visual Studio has no C++ tools. Install them (docs/setup/DEV.md).")
    return Path(output.splitlines()[0])


def load_visual_studio_environment() -> dict[str, str]:
    """Returns the environment of a Visual Studio developer prompt."""
    vcvars = visual_studio_path() / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    completed = subprocess.run(
        f'call "{vcvars}" >nul 2>&1 && set', shell=True, capture_output=True, text=True, errors="replace", check=False
    )
    if completed.returncode != 0:
        raise StepFailed("Could not load the Visual Studio environment.")
    environment = {}
    for line in completed.stdout.splitlines():
        name, separator, value = line.partition("=")
        if separator and name:
            environment[name] = value
    return environment


def build_environment() -> dict[str, str]:
    environment = dict(os.environ)
    if IS_WINDOWS and shutil.which("cl") is None:
        environment = load_visual_studio_environment()
    if not environment.get("VCPKG_ROOT"):
        raise StepFailed("The VCPKG_ROOT variable is not set (docs/setup/DEV.md).")
    return environment


class Output:
    """Where the output of the commands goes. In summary mode, it is saved to a log and only its numbers are shown."""

    def __init__(self, summary: bool, log: Path | None = None) -> None:
        self.summary = summary
        self.log = log
        self.log_started = False

    def save(self, command: Sequence[str], text: str) -> None:
        if self.log is None:
            return
        self.log.parent.mkdir(parents=True, exist_ok=True)
        with self.log.open("a" if self.log_started else "w", encoding="utf-8") as log:
            log.write(f"$ {' '.join(command)}\n{text}\n")
        self.log_started = True


OUTPUT = Output(summary=False)
LOG_PATH = ROOT / "build" / "dev-last.log"

_SUMMARY_LINES = (
    re.compile(r"tests passed"),  # CTest
    re.compile(r"^Ran \d+ tests? in "),  # unittest
    re.compile(r"tests: \d+ of \d+ passed"),  # the PowerShell tests of the hooks
    re.compile(r"^OK: "),  # the repository checks
    re.compile(r"^ninja: no work to do"),
)
_BUILD_PROGRESS = re.compile(r"^\[\d+/(?P<total>\d+)\]")
_WARNING = re.compile(r"\bwarning\b", re.IGNORECASE)


def summarize(text: str) -> list[str]:
    """The numbers of a command that passed: tests passed, build steps and lines with warnings."""
    lines = text.splitlines()
    summary = [line.strip() for line in lines if any(pattern.search(line) for pattern in _SUMMARY_LINES)]
    progress = [match for line in lines if (match := _BUILD_PROGRESS.match(line))]
    if progress:
        summary.append(f"compiled: {progress[-1].group('total')} build step(s)")
    warnings = sum(1 for line in lines if _WARNING.search(line))
    if warnings:
        summary.append(f"{warnings} line(s) with a warning: see {LOG_PATH.relative_to(ROOT).as_posix()}")
    return summary


def run(command: Sequence[str], environment: dict[str, str] | None = None) -> None:
    if not OUTPUT.summary:
        completed = subprocess.run(list(command), cwd=ROOT, env=environment, check=False)
    else:
        completed = subprocess.run(
            list(command), cwd=ROOT, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, errors="replace", check=False,
        )
        text = completed.stdout or ""
        OUTPUT.save(command, text)
        if completed.returncode != 0:
            print(text, end="" if text.endswith("\n") else "\n", flush=True)
        else:
            for line in summarize(text):
                print(f"  {line}", flush=True)
    if completed.returncode != 0:
        raise StepFailed(f"Command failed with exit code {completed.returncode}: {' '.join(command)}")


def find_llvm_tool(name: str, environment: dict[str, str]) -> str:
    found = shutil.which(name, path=environment.get("PATH"))
    if found:
        return found
    if IS_WINDOWS:
        bundled = visual_studio_path() / "VC" / "Tools" / "Llvm" / "x64" / "bin" / f"{name}.exe"
        if bundled.exists():
            return str(bundled)
    raise StepFailed(f"{name} was not found (docs/setup/DEV.md).")


def cpp_files(folders: Sequence[str] = (), sources_only: bool = False) -> list[str]:
    patterns = ["*.cpp"] if sources_only else ["*.cpp", "*.hpp"]
    listed = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", *patterns],
        cwd=ROOT, capture_output=True, text=True, check=True, env=without_git_variables(dict(os.environ)),
    ).stdout.splitlines()
    existing = [path for path in listed if (ROOT / path).exists()]
    if not folders:
        return existing
    return [path for path in existing if path.split("/", 1)[0] in folders]


class Builder:
    """Configures, builds and tests one kind of build with the chosen compiler."""

    def __init__(self, compiler: str) -> None:
        self.compiler = compiler
        self.configured: set[str] = set()

    def preset(self, kind: str) -> str:
        return preset_name(SYSTEM, self.compiler, kind)

    def configure(self, kind: str = "dev") -> None:
        run(["cmake", "--preset", self.preset(kind)], build_environment())
        self.configured.add(kind)

    def configure_once(self, kind: str = "dev") -> None:
        """One command configures each kind of build only once; CMake itself notices later changes while building."""
        if kind not in self.configured:
            self.configure(kind)

    def build(self, kind: str = "dev") -> None:
        self.configure_once(kind)
        run(["cmake", "--build", "--preset", self.preset(kind)], build_environment())

    def test(self, kind: str = "dev") -> None:
        self.build(kind)
        run(["ctest", "--preset", self.preset(kind)], build_environment())

    def sanitize(self) -> None:
        if not has_sanitizers(SYSTEM, self.compiler):
            print("The sanitizers do not run with this system and compiler; skipped.")
            return
        self.test("asan")

    def release(self) -> None:
        if self.compiler != "default":
            print("The player build uses the default compiler of the system; skipped.")
            return
        self.test("release")

    def tidy(self) -> None:
        environment = build_environment()
        self.configure_once("dev")
        tool = find_llvm_tool("clang-tidy", environment)
        build_folder = ROOT / "build" / self.preset("dev")
        files = [path for path in cpp_files(TIDY_FOLDERS, sources_only=True) if "/Tests/" not in path]

        def analyze(path: str) -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [tool, "--quiet", "-p", str(build_folder), "--extra-arg=-Wno-unused-command-line-argument", path],
                cwd=ROOT, env=environment, capture_output=True, text=True, errors="replace", check=False,
            )

        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
            results = list(pool.map(analyze, files))

        failed = [result for result in results if result.returncode != 0]
        for result in failed:
            print(result.stdout, result.stderr, sep="\n")
        if failed:
            raise StepFailed(f"Static analysis found problems in {len(failed)} file(s).")
        print(f"Static analysis: {len(files)} file(s) analyzed, no problems.")


def format_code(fix: bool = False) -> None:
    environment = dict(os.environ) if not IS_WINDOWS else build_environment()
    tool = find_llvm_tool("clang-format", environment)
    files = cpp_files()
    mode = ["-i"] if fix else ["--dry-run", "--Werror"]
    for start in range(0, len(files), 40):
        run([tool, *mode, *files[start:start + 40]])


def checks(names: Sequence[str] = ("all",)) -> None:
    run([sys.executable, "scripts/checks/run.py", *names])


def pytests() -> None:
    environment = without_git_variables(dict(os.environ))
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    for folder in ("scripts/checks/tests", "tests/hooks"):
        if (ROOT / folder).is_dir():
            run([sys.executable, "-m", "unittest", "discover", "-s", folder, "-p", "test_*.py"], environment)
    if IS_WINDOWS:
        for script in sorted((ROOT / "tests" / "hooks").glob("*.tests.ps1")):
            run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script)], environment)


def vcpkg_baseline() -> None:
    configuration = json.loads((ROOT / "vcpkg-configuration.json").read_text(encoding="utf-8"))
    print(configuration["default-registry"]["baseline"])


def run_steps(steps: Sequence[tuple[str, Callable[[], None]]]) -> int:
    finished: list[tuple[str, float]] = []
    for name, step in steps:
        print(f"\n==> {name}", flush=True)
        started = time.monotonic()
        try:
            step()
        except StepFailed as failure:
            print(f"\nFAILED: {name}\n{failure}")
            if OUTPUT.log_started and OUTPUT.log is not None:
                print(f"Full output: {OUTPUT.log.relative_to(ROOT).as_posix()}")
            return 1
        finished.append((name, time.monotonic() - started))
    print("\nResult:")
    for name, seconds in finished:
        print(f"  OK  {name} ({seconds:.1f}s)")
    if OUTPUT.log_started and OUTPUT.log is not None:
        print(f"Full output: {OUTPUT.log.relative_to(ROOT).as_posix()}")
    return 0


def quick_steps(builder: Builder) -> list[tuple[str, Callable[[], None]]]:
    return [
        ("Formatting", format_code),
        ("Build and tests", builder.test),
        ("Repository checks", checks),
        ("Tests of the checks and hooks", pytests),
    ]


def build_only_steps(builder: Builder) -> list[tuple[str, Callable[[], None]]]:
    """What the CI runs on every system. Formatting, static analysis and the player build have jobs of their own."""
    return [
        ("Build and tests", builder.test),
        ("Build and tests with sanitizers", builder.sanitize),
        ("Repository checks", checks),
        ("Tests of the checks and hooks", pytests),
    ]


def verify_steps(builder: Builder) -> list[tuple[str, Callable[[], None]]]:
    return [
        ("Formatting", format_code),
        ("Build and tests", builder.test),
        ("Build and tests with sanitizers", builder.sanitize),
        ("Player build and tests", builder.release),
        ("Static analysis", builder.tidy),
        ("Repository checks", checks),
        ("Tests of the checks and hooks", pytests),
    ]


def main(arguments: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description="Builds and verifies the Nexum.")
    parser.add_argument("command", choices=[
        "verify", "quick", "configure", "build", "test", "sanitize", "release", "format", "tidy", "checks",
        "pytests", "vcpkg-baseline",
    ])
    parser.add_argument("--compiler", choices=["default", "gcc"], default="default",
                        help="gcc: use GCC instead of Clang (Linux only)")
    parser.add_argument("--fix", action="store_true", help="format: correct the files")
    parser.add_argument("--no-static", action="store_true", help="verify: skip formatting, analysis and release")
    parser.add_argument("--full", action="store_true", help="show the whole output of every command, as in the CI")
    parser.add_argument("names", nargs="*", help="checks: which checks to run")
    options = parser.parse_args(arguments)
    builder = Builder(options.compiler)
    if not options.full and not os.environ.get("CI"):
        OUTPUT.summary = True
        OUTPUT.log = LOG_PATH

    simple: dict[str, Callable[[], None]] = {
        "configure": builder.configure, "build": builder.build, "test": builder.test, "sanitize": builder.sanitize,
        "release": builder.release, "tidy": builder.tidy, "pytests": pytests,
    }
    if options.command == "verify":
        return run_steps(build_only_steps(builder) if options.no_static else verify_steps(builder))
    if options.command == "quick":
        return run_steps(quick_steps(builder))
    if options.command == "vcpkg-baseline":
        vcpkg_baseline()
        return 0
    if options.command == "format":
        return run_steps([("Formatting", lambda: format_code(options.fix))])
    if options.command == "checks":
        return run_steps([("Repository checks", lambda: checks(options.names or ("all",)))])
    return run_steps([(options.command, simple[options.command])])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
