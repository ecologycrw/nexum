"""Writes the safety summary of a pull request: protected files that changed, tests that were removed and each new
NOLINT, which turns off a check of the static analysis (P-0015).

The text is in Portuguese because the owner of the product reads it in the PR.
It never fails the build; it only makes these changes impossible to miss.
"""

from __future__ import annotations

import os
import re
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
# Every file but the documents, the Python of Tools/ included (the template of packetgen writes C++).
CODE_PATHSPECS = (".", ":(exclude)*.md")
_HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(?P<start>\d+)(?:,\d+)? @@")
_LONGEST_QUOTED_LINE = 160


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


def _count_powershell_lines_removed(diff: str) -> int:
    """The PowerShell tests of the hooks list their cases as lines, with no marker; every removed line is counted."""
    current = ""
    removed = 0
    for line in diff.splitlines():
        if line.startswith(("--- a/", "+++ b/")):
            current = line[6:]
            continue
        if line.startswith(("---", "+++")) or not line.startswith("-") or not current.endswith(".ps1"):
            continue
        code = line[1:].strip()
        removed += bool(code) and not code.startswith("#")
    return removed


def _is_test_file(path: str) -> bool:
    is_in_tests = path.startswith("tests/") or "/Tests/" in path
    return is_in_tests and PurePosixPath(path).suffix in (".cpp", ".py", ".ps1")


def find_new_nolints(diff: str) -> list[tuple[str, int, str]]:
    """Each added line with NOLINT outside the documents, from a diff with no context: the file, the line, the text."""
    found: list[tuple[str, int, str]] = []
    current = ""
    line_number = 0
    in_header = False
    for line in diff.splitlines():
        if line.startswith("diff --git "):
            current, in_header = "", True
            continue
        hunk = _HUNK.match(line)
        if hunk:
            line_number, in_header = int(hunk.group("start")), False
            continue
        if in_header:
            if line.startswith("+++ "):
                current = line[6:] if line.startswith("+++ b/") else ""
            continue
        if not line.startswith("+"):
            continue
        if current and not current.endswith(".md") and "NOLINT" in line:
            found.append((current, line_number, line[1:].strip().replace("`", "'")[:_LONGEST_QUOTED_LINE]))
        line_number += 1
    return found


def build(changes: list[tuple[str, str]], test_diff: str, code_diff: str = "") -> str:
    protected = [change for change in changes if is_protected(change[1])]
    deleted_tests = [path for status, path in changes if status == "D" and _is_test_file(path)]
    added, removed = _count_tests(test_diff)
    powershell_removed = _count_powershell_lines_removed(test_diff)

    lines = ["## Resumo de segurança do PR", "", f"### Travas alteradas ({len(protected)})", ""]
    if protected:
        lines += [f"- `{path}` ({_DESCRIPTIONS.get(status, 'alterado')})" for status, path in protected]
        lines += ["", "> **Atenção:** mudanças em travas exigem a aprovação explícita do dono (AGENTS.md)."]
    else:
        lines.append("Nenhuma.")

    lines += ["", "### Testes", "", f"- Testes adicionados: {added}", f"- Testes removidos: {removed}",
              f"- Arquivos de teste apagados: {len(deleted_tests)}"]
    lines += [f"  - `{path}`" for path in deleted_tests]
    lines.append(f"- Linhas removidas dos testes em PowerShell (os casos dos hooks): {powershell_removed}")
    if removed or deleted_tests:
        lines += ["", "> **Atenção:** testes foram removidos. O report da fatia precisa explicar o motivo de cada remoção."]
    if powershell_removed:
        lines += ["", "> **Atenção:** linhas saíram dos testes em PowerShell. Confira se algum caso de teste foi removido "
                      "ou enfraquecido; se foi, o report precisa explicar o motivo."]

    nolints = find_new_nolints(code_diff)
    lines += ["", f"### `NOLINT` novos ({len(nolints)})", ""]
    if nolints:
        lines += [f"- `{path}`, linha {number}: `{text}`" for path, number, text in nolints]
        lines += ["", "> **Atenção:** cada `NOLINT` novo desliga uma conferência da análise estática. Confira se o "
                      "motivo, no comentário da linha de cima, justifica a exceção."]
    else:
        lines.append("Nenhum.")
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
    code_diff = _git(root, "diff", "-U0", revision_range, "--", *CODE_PATHSPECS)
    if name_status is None or test_diff is None or code_diff is None:
        print(f"git diff failed for '{revision_range}'.")
        return 1
    summary = build(parse_name_status(name_status), test_diff, code_diff)
    print(summary, end="")
    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_file:
        with open(summary_file, "a", encoding="utf-8") as output:
            output.write(summary)
    return 0
