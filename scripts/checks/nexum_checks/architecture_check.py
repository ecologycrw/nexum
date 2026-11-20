"""Keeps the layers apart and each external library behind its module (INV-02 and INV-10).

Two kinds of rules:
- layers: Engine never depends on Kit or Games; Kit never depends on Games; Engine/Core depends on no other module;
- owners: SDL, the operating system and the other engine libraries are used only in the Private folder of the
  module that owns them. Public headers never expose them.

Every C++ file and every CMake file of Engine, Kit, Games and tests is checked. Tools may use anything.
"""

from __future__ import annotations

import re
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "architecture"

_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"](?P<header>[^>"]+)[>"]', re.MULTILINE)
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//[^\n]*")
_CMAKE_COMMENT = re.compile(r"#[^\n]*")

# Letter case is ignored: on Windows, an include written with the wrong case still finds the file.
_HEADER_GROUPS = {
    "kit": re.compile(r"^nexum/gridrpg/", re.IGNORECASE),
    "game": re.compile(r"^mu/", re.IGNORECASE),
    "engine-module": re.compile(r"^nexum/(?!core/|gridrpg/)[a-z0-9]+/", re.IGNORECASE),
    "sdl": re.compile(r"^SDL3?(_\w+)?(/|\.h$)|^SDL_", re.IGNORECASE),
    "os": re.compile(
        r"^(windows\.h|winsock2?\.h|ws2tcpip\.h|winuser\.h|d3d\d*\.h|dxgi\d*\.h|io\.h|direct\.h|process\.h"
        r"|unistd\.h|pthread\.h|dlfcn\.h|fcntl\.h|poll\.h|netdb\.h|sys/.*|arpa/.*|netinet/.*|linux/.*|mach/.*"
        r"|CoreFoundation/.*|Metal/.*)$",
        re.IGNORECASE,
    ),
    "asio": re.compile(r"^asio(\.hpp$|/)", re.IGNORECASE),
    "entt": re.compile(r"^entt/", re.IGNORECASE),
    "rmlui": re.compile(r"^rmlui/", re.IGNORECASE),
    "imgui": re.compile(r"^imgui(\.h$|_|/)", re.IGNORECASE),
    "tracy": re.compile(r"^tracy/", re.IGNORECASE),
    "miniaudio": re.compile(r"^miniaudio\.h$", re.IGNORECASE),
    "glm": re.compile(r"^glm/", re.IGNORECASE),
}

_TARGET_GROUPS = {
    "kit": re.compile(r"\b(nexum_gridrpg\w*|Nexum::GridRpg)\b"),
    "game": re.compile(r"\b(MuOnline|MuClient|muonline_\w+)\b"),
    "sdl": re.compile(r"\bSDL3(_\w+)?::"),
    "asio": re.compile(r"\basio::"),
    "entt": re.compile(r"\bEnTT::"),
    "rmlui": re.compile(r"\bRmlUi::"),
    "imgui": re.compile(r"\bimgui::"),
    "tracy": re.compile(r"\bTracy::"),
    "glm": re.compile(r"\bglm::"),
}

# Each library and the places where it may appear: the Private folder and the CMakeLists.txt of its module.
_OWNERS = {
    "sdl": ("Engine/Platform/Private/", "Engine/Platform/CMakeLists.txt",
            "Engine/Render/Private/", "Engine/Render/CMakeLists.txt"),
    "os": ("Engine/Platform/Private/",),
    "asio": ("Engine/Net/Private/", "Engine/Net/CMakeLists.txt"),
    "entt": ("Engine/World/Private/", "Engine/World/CMakeLists.txt"),
    "rmlui": ("Engine/Ui/Private/", "Engine/Ui/CMakeLists.txt"),
    "imgui": ("Engine/Debug/Private/", "Engine/Debug/CMakeLists.txt"),
    "tracy": ("Engine/Debug/Private/", "Engine/Debug/CMakeLists.txt"),
    "miniaudio": ("Engine/Audio/Private/", "Engine/Audio/CMakeLists.txt"),
    "glm": ("Engine/Core/Private/", "Engine/Core/CMakeLists.txt"),
}

_LAYER_REASONS = {
    "kit": "must not depend on the kit: the dependencies only go down (INV-02)",
    "game": "must not depend on the MU module: the dependencies only go down (INV-02)",
    "engine-module": "must not depend on other engine modules: Engine/Core is the base of everything (INV-02)",
}

# What each part of the repository cannot depend on. Parts are tried in order: the first match wins.
_LAYERS: tuple[tuple[str, frozenset[str]], ...] = (
    ("Engine/Core/", frozenset({"kit", "game", "engine-module"})),
    ("Engine/", frozenset({"kit", "game"})),
    ("Kit/", frozenset({"game"})),
    ("tests/architecture/OtherGame/", frozenset({"game"})),
)

_SCANNED_FOLDERS = ("Engine", "Kit", "Games", "tests")


def forbidden_for(path: str) -> frozenset[str]:
    """The groups a file cannot use: the layers above it and the libraries it does not own."""
    forbidden: set[str] = set()
    for prefix, groups in _LAYERS:
        if path.startswith(prefix):
            forbidden |= groups
            break
    for library, owners in _OWNERS.items():
        if not any(path.startswith(owner) for owner in owners):
            forbidden.add(library)
    return frozenset(forbidden)


def _reason(group: str) -> str:
    if group in _LAYER_REASONS:
        return _LAYER_REASONS[group]
    owners = ", ".join(owner for owner in _OWNERS[group] if owner.endswith("/"))
    return f"must not use {group}: only {owners} may (INV-10)"


def _without_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", text))


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for folder in _SCANNED_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h"):
            _check_includes(root, file, findings)
        for file in iter_files(root / folder, "CMakeLists.txt", ".cmake"):
            _check_targets(root, file, findings)
    return findings


def _check_includes(root: Path, file: Path, findings: list[Finding]) -> None:
    where = relative(root, file)
    forbidden = forbidden_for(where)
    for match in _INCLUDE.finditer(_without_comments(read_text(file))):
        header = match.group("header")
        for group in sorted(forbidden):
            if _HEADER_GROUPS[group].search(header):
                findings.append(Finding(NAME, where, f"includes <{header}>: this code {_reason(group)}"))


def _check_targets(root: Path, file: Path, findings: list[Finding]) -> None:
    where = relative(root, file)
    forbidden = forbidden_for(where)
    text = _CMAKE_COMMENT.sub("", read_text(file))
    for group in sorted(forbidden & _TARGET_GROUPS.keys()):
        if _TARGET_GROUPS[group].search(text):
            findings.append(Finding(NAME, where, f"links a forbidden target: this code {_reason(group)}"))
