"""Finding and reading the files of the repository."""

from __future__ import annotations

from collections.abc import Iterator
from pathlib import Path

MARKER_FILE = "AGENTS.md"
IGNORED_FOLDERS = frozenset(
    {
        ".git", ".vs", ".idea", ".vscode", "build", "out", "vcpkg_installed", "node_modules", "__pycache__",
        "Testing", "cache",
    }
)


def find_root(start: Path) -> Path | None:
    for folder in (start, *start.parents):
        if (folder / MARKER_FILE).is_file():
            return folder
    return None


def iter_files(root: Path, *suffixes: str) -> Iterator[Path]:
    """Every file below the folder, in a stable order. With suffixes, only the files that end with them."""
    if not root.is_dir():
        return
    for entry in sorted(root.iterdir(), key=lambda item: item.name):
        if entry.is_dir():
            if entry.name not in IGNORED_FOLDERS:
                yield from iter_files(entry, *suffixes)
        elif not suffixes or entry.name.endswith(suffixes):
            yield entry


def relative(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")
