"""Command line of the checks.

Exit codes: 0 = no problems, 1 = problems found, 2 = usage error.
"""

from __future__ import annotations

import argparse
from collections.abc import Callable, Sequence
from pathlib import Path

from . import architecture_check, cpp_rules_check, docs_check, pr_summary, structure_check, terms_check
from .findings import Finding
from .repository import MARKER_FILE, find_root

CHECKS: dict[str, Callable[[Path], list[Finding]]] = {
    "docs": docs_check.run,
    "structure": structure_check.run,
    "architecture": architecture_check.run,
    "cpp-rules": cpp_rules_check.run,
    "terms": terms_check.run,
}


def main(arguments: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(prog="scripts/checks/run.py", description="Runs the repository checks.")
    parser.add_argument("commands", nargs="+", choices=[*CHECKS, "all", "pr-summary"])
    parser.add_argument("--root", type=Path, help="the repository folder")
    parser.add_argument("--base", default="origin/main", help="pr-summary: the branch to compare with")
    options = parser.parse_args(arguments)

    root = options.root or find_root(Path.cwd())
    if root is None:
        print(f"Could not find the repository root ({MARKER_FILE}).")
        return 2

    if "pr-summary" in options.commands:
        return pr_summary.run(root, options.base)

    names = list(CHECKS) if "all" in options.commands else list(dict.fromkeys(options.commands))
    findings = sorted(finding for name in names for finding in CHECKS[name](root))
    for finding in findings:
        print(finding)
    print(f"OK: {', '.join(names)}" if not findings else f"{len(findings)} problem(s) found.")
    return 0 if not findings else 1
