import contextlib
import importlib.util
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

import support  # noqa: F401

ROOT = Path(__file__).resolve().parents[3]


def load_dev():
    spec = importlib.util.spec_from_file_location("nexum_dev", ROOT / "scripts" / "dev.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


dev = load_dev()


class PresetTests(unittest.TestCase):
    def configure_presets(self) -> set[str]:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        return {preset["name"] for preset in presets["configurePresets"] if not preset.get("hidden")}

    def test_each_system_gets_its_own_presets(self) -> None:
        self.assertEqual(dev.system_name("Windows"), "windows")
        self.assertEqual(dev.system_name("Linux"), "linux")
        self.assertEqual(dev.system_name("Darwin"), "macos")

    def test_every_preset_the_script_asks_for_exists(self) -> None:
        existing = self.configure_presets()
        wanted = {dev.preset_name(system, "default", kind)
                  for system in ("windows", "linux", "macos") for kind in ("dev", "release")}
        wanted |= {dev.preset_name(system, "default", "asan") for system in ("windows", "linux")
                   if dev.has_sanitizers(system, "default")}
        wanted.add(dev.preset_name("linux", "gcc", "dev"))
        self.assertEqual(wanted - existing, set())

    def test_every_preset_has_a_build_and_a_test_preset(self) -> None:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        for kind in ("buildPresets", "testPresets"):
            with self.subTest(kind=kind):
                names = {preset["name"] for preset in presets[kind] if not preset.get("hidden")}
                self.assertEqual(self.configure_presets() - names, set())

    def test_the_player_presets_turn_the_authoring_mode_off(self) -> None:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        player = next(preset for preset in presets["configurePresets"] if preset["name"] == "player")
        self.assertEqual(player["cacheVariables"]["NEXUM_AUTHORING"], "OFF")
        self.assertEqual(player["cacheVariables"]["CMAKE_BUILD_TYPE"], "Release")
        for system in ("windows", "linux", "macos"):
            release = next(preset for preset in presets["configurePresets"] if preset["name"] == f"{system}-release")
            self.assertIn("player", release["inherits"])

    def test_gcc_is_used_only_on_linux_for_the_dev_build(self) -> None:
        self.assertEqual(dev.preset_name("linux", "gcc", "dev"), "linux-gcc-dev")
        for system, kind in (("windows", "dev"), ("macos", "dev"), ("linux", "asan"), ("linux", "release")):
            with self.subTest(system=system, kind=kind):
                with self.assertRaises(dev.StepFailed):
                    dev.preset_name(system, "gcc", kind)

    def test_the_sanitizers_run_only_where_they_have_a_preset(self) -> None:
        self.assertTrue(dev.has_sanitizers("windows", "default"))
        self.assertTrue(dev.has_sanitizers("linux", "default"))
        self.assertFalse(dev.has_sanitizers("macos", "default"))
        self.assertFalse(dev.has_sanitizers("linux", "gcc"))

    def test_vcpkg_does_not_print_its_usage_notes(self) -> None:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        base = next(preset for preset in presets["configurePresets"] if preset["name"] == "base")
        self.assertIn("--no-print-usage", base["cacheVariables"]["VCPKG_INSTALL_OPTIONS"])


class SummaryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.log = Path(self.folder.name) / "dev-last.log"
        self.previous = dev.OUTPUT
        dev.OUTPUT = dev.Output(summary=True, log=self.log)
        self.addCleanup(setattr, dev, "OUTPUT", self.previous)

    def run_python(self, code: str) -> str:
        shown = io.StringIO()
        with contextlib.redirect_stdout(shown):
            dev.run([sys.executable, "-c", code])
        return shown.getvalue()

    def test_the_summary_keeps_the_numbers_of_each_kind_of_step(self) -> None:
        text = "\n".join([
            "-- Running vcpkg install", "[1/3] Building CXX object a.obj", "[3/3] Linking CXX executable a.exe",
            "      Start  1: core", " 1/2 Test  #1: core .... Passed", "100% tests passed, 0 tests failed out of 2",
            "Ran 160 tests in 7.156s", "guard-bash tests: 98 of 98 passed.", "OK: docs, terms",
            "a.cpp(3): warning C4100: unreferenced parameter",
        ])
        self.assertEqual(dev.summarize(text), [
            "100% tests passed, 0 tests failed out of 2", "Ran 160 tests in 7.156s",
            "guard-bash tests: 98 of 98 passed.", "OK: docs, terms", "compiled: 3 build step(s)",
            "1 line(s) with a warning: see build/dev-last.log",
        ])

    def test_a_command_that_passes_shows_only_its_numbers_and_saves_the_rest(self) -> None:
        shown = self.run_python("print('noise line'); print('100% tests passed, 0 tests failed out of 4')")
        self.assertEqual(shown, "  100% tests passed, 0 tests failed out of 4\n")
        self.assertIn("noise line", self.log.read_text(encoding="utf-8"))

    def test_a_command_that_fails_is_shown_in_full(self) -> None:
        shown = io.StringIO()
        code = "import sys; print('first line'); print('a.cpp(9): error C2065: undeclared', file=sys.stderr); sys.exit(3)"
        with contextlib.redirect_stdout(shown), self.assertRaises(dev.StepFailed) as failure:
            dev.run([sys.executable, "-c", code])
        self.assertIn("first line", shown.getvalue())
        self.assertIn("error C2065", shown.getvalue())
        self.assertIn("exit code 3", str(failure.exception))

    def test_the_log_holds_every_command_of_one_run(self) -> None:
        self.run_python("print('one')")
        self.run_python("print('two')")
        log = self.log.read_text(encoding="utf-8")
        self.assertIn("one", log)
        self.assertIn("two", log)

    def test_each_kind_of_build_is_configured_once(self) -> None:
        builder = dev.Builder("default")
        configured = []
        builder.configure = lambda kind="dev": (configured.append(kind), builder.configured.add(kind))
        builder.configure_once("dev")
        builder.configure_once("dev")
        builder.configure_once("release")
        self.assertEqual(configured, ["dev", "release"])


if __name__ == "__main__":
    unittest.main()
