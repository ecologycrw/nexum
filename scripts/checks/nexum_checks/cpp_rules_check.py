"""Refuses dangerous C++ that the compiler accepts (INV-05, INV-07, INV-11, INV-12 and INV-14, docs/CONVENTIONS.md).

Each rule lists the only folders where the construct is allowed. An empty list means it is forbidden everywhere.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "cpp-rules"

_SCANNED_FOLDERS = ("Engine", "Kit", "Games", "Tools", "tests")
_EXEMPT_FOLDER = "tests/architecture/sanitizer_canary/"
# Counting allocations (INV-13) requires replacing the global allocation functions. Every other rule still applies.
_ALLOCATION_COUNTER_FOLDER = "tests/architecture/frame_allocations/"


@dataclass(frozen=True)
class Rule:
    pattern: re.Pattern[str]
    message: str
    allowed_in: tuple[str, ...] = ()


_MEMORY_RULES = (
    Rule(re.compile(r"\bnew\b"), "'new' is forbidden: use std::make_unique or a container (INV-11)"),
    Rule(re.compile(r"\b(malloc|calloc|realloc|free)\s*\("), "manual memory management is forbidden (INV-11)"),
)

_OTHER_RULES = (
    Rule(
        re.compile(r"\b(strcpy|strncpy|strcat|strncat|sprintf|vsprintf|gets|scanf|sscanf|fscanf)\s*\("),
        "C string functions without bounds are forbidden: use std::string or std::span",
    ),
    Rule(re.compile(r"\b(rand|srand)\s*\("), "the system rand() is forbidden: use the engine's generator (INV-07)"),
    Rule(
        re.compile(r"\brandom_device\b"),
        "random numbers must come from the engine's generator (INV-07)",
        ("Engine/Platform/Private/",),
    ),
    Rule(
        re.compile(r"\b(system_clock|steady_clock|high_resolution_clock)\b"),
        "time must come from the engine's clocks (INV-05)",
        ("Engine/Time/Private/", "Tools/"),
    ),
    Rule(
        re.compile(r"\breinterpret_cast\b"),
        "never reinterpret bytes: read external data with a bounds-checked reader (INV-12)",
        ("Engine/Platform/Private/",),
    ),
    Rule(
        re.compile(r"\bthrow\b"),
        "exceptions never leave a runtime module: return a Result (INV-14)",
        ("Tools/", "tests/"),
    ),
)

_DELETE = re.compile(r"\bdelete\b")
# The standard header <new> brings std::launder and the cache line sizes; including it allocates nothing.
_INCLUDE_NEW = re.compile(r"^[ \t]*#[ \t]*include[ \t]*<new>", re.MULTILINE)
_CODE_PARTS = re.compile(
    r"""
    (?P<raw>R"(?P<delimiter>[^()\s]{0,16})\(.*?\)(?P=delimiter)")
    | (?P<block>/\*.*?\*/)
    | (?P<line>//[^\n]*)
    | (?P<string>"(?:\\.|[^"\\\n])*")
    | (?P<char>'(?:\\.|[^'\\\n])+')
    """,
    re.DOTALL | re.VERBOSE,
)


def strip_comments_and_strings(text: str) -> str:
    """Blanks comments and string contents, keeping the line numbers."""

    def blank(match: re.Match[str]) -> str:
        return "".join("\n" if character == "\n" else " " for character in match.group(0))

    return _CODE_PARTS.sub(blank, text)


def _line_of(text: str, position: int) -> int:
    return text.count("\n", 0, position) + 1


def _is_real_delete(code: str, position: int) -> bool:
    """"= delete" removes a function and is allowed."""
    return not code[:position].rstrip().endswith("=")


def _rules_for(where: str) -> tuple[Rule, ...]:
    rules = _OTHER_RULES if where.startswith(_ALLOCATION_COUNTER_FOLDER) else _MEMORY_RULES + _OTHER_RULES
    return tuple(rule for rule in rules if not any(where.startswith(place) for place in rule.allowed_in))


def _without_include_new(code: str) -> str:
    return _INCLUDE_NEW.sub(lambda match: " " * len(match.group(0)), code)


def check_file(where: str, text: str) -> list[Finding]:
    code = _without_include_new(strip_comments_and_strings(text))
    findings = [
        Finding(NAME, where, f"line {_line_of(code, match.start())}: {rule.message}")
        for rule in _rules_for(where)
        for match in rule.pattern.finditer(code)
    ]
    if where.startswith(_ALLOCATION_COUNTER_FOLDER):
        return findings
    findings += [
        Finding(NAME, where, f"line {_line_of(code, match.start())}: 'delete' is forbidden: use std::unique_ptr (INV-11)")
        for match in _DELETE.finditer(code)
        if _is_real_delete(code, match.start())
    ]
    return findings


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for folder in _SCANNED_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h"):
            where = relative(root, file)
            if not where.startswith(_EXEMPT_FOLDER):
                findings.extend(check_file(where, read_text(file)))
    return findings
