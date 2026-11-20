"""Writes the safety summary of a pull request: protected files that changed and tests that were removed.

The text is in Portuguese because the owner of the product reads it in the PR.
It never fails the build; it only makes these changes impossible to miss.
"""

from __future__ import annotations

import os
import subprocess
from pathlib import Path, PurePosixPath

PROTECTED_PREFIXES = (
    ".claude/", ".githooks/", ".github/", "cmake/", "scripts/checks/", "tests/architecture/", "tests/hooks/",
    "tests/visual/approved/", "docs/referencia/",
)
PROTECTED_FILES = (
    "AGENTS.md", "CLAUDE.md", "CMakePresets.json", "vcpkg.json", "vcpkg-configuration.json", ".clang-format",
    ".clang-tidy", ".editorconfig", "scripts/dev.py", "docs/VISION.md", "docs/ROADMAP.md", "docs/VISUAL.md",
)
_DESCRIPTIONS = {"A": "criado", "D": "apagado", "R": "renomeado"}
# doctest: TEST_CASE, TEST_CASE_FIXTURE, TEST_CASE_TEMPLATE, SCENARIO and SUBCASE; Python: unittest methods.
_TEST_MARKERS = ("TEST_CASE(", "TEST_CASE_FIXTURE(", "TEST_CASE_TEMPLATE(", "SCENARIO(", "SUBCASE(", "def test_")
TEST_PATHSPECS = ("tests/", "scripts/checks/tests/", ":(glob)**/Tests/**")


def is_protected(path: str) -> bool:
    return path in PROTECTED_FILES or path.startswith(PROTECTED_PREFIXES)


def parse_name_status(text: str) -> list[tuple[str, str]]:
    changes: list[tuple[str, str]] = []
    for line in text.splitlines():
        parts = line.split("\t")
        if len(parts) > 1:
            changes.extend((parts[0][:1], path) for path in parts[1:])
    return changes


def _count_tests(diff: str) -> tuple[int, int]:
    added = removed = 0
    for line in diff.splitlines():
        if line.startswith(("+++", "---")) or not any(marker in line for marker in _TEST_MARKERS):
            continue
        added += line.startswith("+")
        removed += line.startswith("-")
    return added, removed


def _is_test_file(path: str) -> bool:
    is_in_tests = path.startswith("tests/") or "/Tests/" in path
    return is_in_tests and PurePosixPath(path).suffix in (".cpp", ".py")


def build(changes: list[tuple[str, str]], test_diff: str) -> str:
    protected = [change for change in changes if is_protected(change[1])]
    deleted_tests = [path for status, path in changes if status == "D" and _is_test_file(path)]
    added, removed = _count_tests(test_diff)

    lines = ["## Resumo de segurança do PR", "", f"### Travas alteradas ({len(protected)})", ""]
    if protected:
        lines += [f"- `{path}` ({_DESCRIPTIONS.get(status, 'alterado')})" for status, path in protected]
        lines += ["", "> **Atenção:** mudanças em travas exigem a aprovação explícita do dono (docs/WORKFLOW.md)."]
    else:
        lines.append("Nenhuma.")

    lines += ["", "### Testes", "", f"- Testes adicionados: {added}", f"- Testes removidos: {removed}",
              f"- Arquivos de teste apagados: {len(deleted_tests)}"]
    lines += [f"  - `{path}`" for path in deleted_tests]
    if removed or deleted_tests:
        lines += ["", "> **Atenção:** testes foram removidos. O report da fatia precisa explicar o motivo de cada remoção."]
    return "\n".join(lines) + "\n"


def _git(root: Path, *arguments: str) -> str | None:
    completed = subprocess.run(
        ["git", *arguments], cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace", check=False
    )
    return completed.stdout if completed.returncode == 0 else None


def run(root: Path, base_ref: str) -> int:
    revision_range = f"{base_ref}...HEAD"
    name_status = _git(root, "diff", "--name-status", revision_range)
    test_diff = _git(root, "diff", "-U0", revision_range, "--", *TEST_PATHSPECS)
    if name_status is None or test_diff is None:
        print(f"git diff failed for '{revision_range}'.")
        return 1
    summary = build(parse_name_status(name_status), test_diff)
    print(summary, end="")
    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_file:
        with open(summary_file, "a", encoding="utf-8") as output:
            output.write(summary)
    return 0
