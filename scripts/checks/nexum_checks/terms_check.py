"""The red line: Engine and Kit know nothing of MU, and only Tools know the original file formats (INV-03, INV-08).

Each line of the terms file is "term" or "term ; forbidden: folder, folder". Without the "forbidden:" part,
the term is forbidden in the red-line folders (RED_LINE). Folders are shell patterns, such as "Engine/*".
"""

from __future__ import annotations

import fnmatch
import re
from dataclasses import dataclass
from pathlib import Path

from .findings import Finding
from .repository import iter_files, relative

NAME = "terms"
TERMS_FILE = "scripts/checks/forbidden_terms.txt"
RED_LINE = ("Engine/*", "Kit/*", "tests/architecture/*")
_FORBIDDEN_PREFIX = "forbidden:"


@dataclass(frozen=True)
class Term:
    text: str
    forbidden: tuple[str, ...]
    pattern: re.Pattern[str]


class TermsFileError(ValueError):
    """A line of the terms file that cannot be read."""


def parse_line(content: str) -> Term:
    text, separator, places = content.partition(";")
    text = text.strip().lower()
    if not text:
        raise TermsFileError(f"empty term in '{content}'")
    forbidden = RED_LINE
    if separator:
        places = places.strip()
        if not places.startswith(_FORBIDDEN_PREFIX):
            raise TermsFileError(f"after ';' write 'forbidden: folder, folder' (found '{places}')")
        forbidden = tuple(place.strip() for place in places[len(_FORBIDDEN_PREFIX):].split(",") if place.strip())
        if not forbidden:
            raise TermsFileError(f"'{text}' has no folder after 'forbidden:'")
    pattern = re.compile(rf"(?<![a-z0-9]){re.escape(text)}(?![a-z0-9])", re.IGNORECASE)
    return Term(text, forbidden, pattern)


def load_terms(root: Path) -> tuple[list[Term], list[Finding]]:
    terms: list[Term] = []
    problems: list[Finding] = []
    file = root / TERMS_FILE
    if not file.is_file():
        return terms, [Finding(NAME, TERMS_FILE, "the terms file is missing")]
    for number, line in enumerate(file.read_text(encoding="utf-8").splitlines(), start=1):
        content = line.strip()
        if not content or content.startswith("#"):
            continue
        try:
            terms.append(parse_line(content))
        except TermsFileError as error:
            problems.append(Finding(NAME, TERMS_FILE, f"line {number}: {error}"))
    return terms, problems


def _is_forbidden(term: Term, where: str) -> bool:
    return any(fnmatch.fnmatch(where, place) for place in term.forbidden)


def run(root: Path) -> list[Finding]:
    terms, findings = load_terms(root)
    for file in iter_files(root):
        where = relative(root, file)
        applicable = [term for term in terms if _is_forbidden(term, where)]
        if not applicable:
            continue
        content = file.read_bytes()
        if b"\0" in content:
            continue
        text = content.decode("utf-8", errors="replace")
        for term in applicable:
            match = term.pattern.search(text)
            if match:
                line = text.count("\n", 0, match.start()) + 1
                findings.append(
                    Finding(NAME, where, f"line {line}: the term '{term.text}' is not allowed here (INV-03, INV-08)")
                )
    return findings
