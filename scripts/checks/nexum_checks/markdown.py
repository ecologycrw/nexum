"""Small Markdown helpers: front matter, links, headings and GitHub-style anchors."""

from __future__ import annotations

import re
import unicodedata
from collections.abc import Iterator

_LINK = re.compile(r'!?\[(?:[^\]\\]|\\.)*\]\((?P<target>[^)\s]+)(?:\s+"[^"]*")?\)')
_HEADING = re.compile(r"^\s{0,3}#{1,6}\s+(?P<text>.+?)\s*#*\s*$")
_INLINE_CODE = re.compile(r"`[^`]*`")
_LINK_TEXT = re.compile(r"\[(?P<text>[^\]]*)\]\([^)]*\)")


def split_lines(text: str) -> list[str]:
    return text.replace("\r\n", "\n").split("\n")


def read_front_matter(text: str) -> dict[str, str]:
    """Reads the "key: value" lines between the two "---" lines at the top. " #" starts a comment."""
    lines = split_lines(text)
    if not lines or lines[0].strip() != "---":
        return {}
    result: dict[str, str] = {}
    for line in lines[1:]:
        if line.strip() == "---":
            break
        key, separator, value = line.partition(":")
        if not separator or not key:
            continue
        result[key.strip()] = value.split(" #", 1)[0].strip()
    return result


def _blank_fences(text: str, remove_inline_code: bool) -> list[str]:
    result: list[str] = []
    fence: str | None = None
    for line in split_lines(text):
        trimmed = line.lstrip()
        if fence is None and trimmed.startswith(("```", "~~~")):
            fence = trimmed[:3]
            result.append("")
        elif fence is not None:
            if trimmed.startswith(fence):
                fence = None
            result.append("")
        else:
            result.append(_INLINE_CODE.sub("", line) if remove_inline_code else line)
    return result


def prose_lines(text: str) -> list[str]:
    """The lines without code blocks and inline code, so examples are never read as real content."""
    return _blank_fences(text, remove_inline_code=True)


def links(text: str) -> Iterator[tuple[str, int]]:
    for number, line in enumerate(prose_lines(text), start=1):
        for match in _LINK.finditer(line):
            yield match.group("target"), number


def slug(heading: str) -> str:
    """GitHub's anchor for a heading: lower case, without punctuation, with spaces turned into hyphens."""
    text = _LINK_TEXT.sub(r"\g<text>", heading).replace("`", "").strip().lower()
    kept = []
    for character in text:
        if character == " ":
            kept.append("-")
        elif character.isalnum() or character in "-_" or unicodedata.category(character).startswith("M"):
            kept.append(character)
    return "".join(kept)


def anchors(text: str) -> set[str]:
    """The anchors of a file. Repeated headings get "-1", "-2"..."""
    found: set[str] = set()
    for line in _blank_fences(text, remove_inline_code=False):
        match = _HEADING.match(line)
        if not match:
            continue
        base = slug(match.group("text"))
        candidate = base
        suffix = 0
        while candidate in found:
            suffix += 1
            candidate = f"{base}-{suffix}"
        found.add(candidate)
    return found
