"""Keeps the repository organized: required files, module layout, file names, the licenses of the libraries,
and no file of the original game in the repository.

The rules come from docs/adr/0022-organizacao-do-repositorio.md, docs/adr/0023-arquivos-e-includes-em-pascalcase.md,
docs/adr/0021-repositorio-publico-mit.md and docs/adr/0020-conteudo-original-e-distribuicao.md.
"""

from __future__ import annotations

import json
import re
from collections.abc import Iterator
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "structure"
THIRD_PARTY_FILE = "docs/THIRD_PARTY.md"
REFERENCE_SPEC = "docs/referencia/especificacao-engine-propria-caminho-b.md"

REQUIRED_FILES = (
    "AGENTS.md", "CLAUDE.md", "README.md", "LICENSE", "SECURITY.md", "CMakeLists.txt", "CMakePresets.json",
    "vcpkg.json", "vcpkg-configuration.json", ".clang-format", ".clang-tidy", ".editorconfig", ".gitattributes",
    ".gitignore", "docs/ARCHITECTURE.md", "docs/CONVENTIONS.md", "docs/GLOSSARY.md", "docs/PROPOSALS.md",
    "docs/ROADMAP.md", "docs/STATUS.md", "docs/TECH_DEBT.md", THIRD_PARTY_FILE, "docs/VISION.md", "docs/VISUAL.md",
    "docs/WORKFLOW.md", "docs/adr/README.md", "docs/referencia/README.md", REFERENCE_SPEC, ".claude/settings.json",
)

# The folders that hold modules, and the include folder of their public headers.
_MODULE_FOLDERS = ("Engine", "Kit", "Games")
_TOOL_FOLDER = "Tools"
_INCLUDE_ROOTS = {"Games/MuOnline": ("Mu",)}
_PRODUCT_FOLDERS = (*_MODULE_FOLDERS, _TOOL_FOLDER)
_CODE_FOLDERS = (*_PRODUCT_FOLDERS, "tests")
# "tools" in lower case is the same folder as Tools on Windows; the scripts live in scripts/.
_RETIRED_FOLDERS = {
    "src": "use Engine, Kit, Games or Tools",
    "include": "use Engine, Kit, Games or Tools",
    "source": "use Engine, Kit, Games or Tools",
    "tools": "use scripts/ for the scripts or Tools/ for the C++ tools",
}
_CPP_NAME = re.compile(r"^[A-Z][A-Za-z0-9]*\.(cpp|hpp)$")
_MODULE_NAME = re.compile(r"^[A-Z][A-Za-z0-9]*$")
_REGISTERED_LIBRARY = re.compile(r"^\|\s*`(?P<name>[^`]+)`\s*\|(?P<license>[^|]*)\|", re.MULTILINE)

# Files of the original game never enter the repository (INV-17).
_ORIGINAL_SUFFIXES = frozenset({".bmd", ".ozj", ".ozt", ".ozb", ".ozd", ".ozp", ".ozg"})
_ORIGINAL_NAME = re.compile(r"^(encterrain\d*\.(map|att|obj)|terrain\d*\.att|terrainheight\.\w+|terrainlight\.\w+)$",
                            re.IGNORECASE)


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for required in REQUIRED_FILES:
        if not (root / required).is_file():
            findings.append(Finding(NAME, required, "this required file is missing"))
    _check_retired_folders(root, findings)
    _check_modules(root, findings)
    _check_tools(root, findings)
    _check_file_names(root, findings)
    _check_test_folders(root, findings)
    _check_third_party(root, findings)
    _check_original_files(root, findings)
    return findings


def _check_original_files(root: Path, findings: list[Finding]) -> None:
    for file in iter_files(root):
        if file.suffix.lower() in _ORIGINAL_SUFFIXES or _ORIGINAL_NAME.match(file.name):
            findings.append(
                Finding(NAME, relative(root, file), "files of the original game never enter the repository (INV-17)")
            )


def _check_third_party(root: Path, findings: list[Finding]) -> None:
    manifest = root / "vcpkg.json"
    registry = root / THIRD_PARTY_FILE
    if not manifest.is_file() or not registry.is_file():
        return  # The missing file was already reported.
    try:
        dependencies = json.loads(read_text(manifest)).get("dependencies", [])
    except json.JSONDecodeError:
        findings.append(Finding(NAME, "vcpkg.json", "the file is not valid JSON"))
        return
    libraries = {item if isinstance(item, str) else item.get("name", "") for item in dependencies}
    licenses = {
        match.group("name"): match.group("license").strip()
        for match in _REGISTERED_LIBRARY.finditer(read_text(registry))
    }
    for library in sorted(library for library in libraries if not licenses.get(library)):
        findings.append(Finding(NAME, THIRD_PARTY_FILE, f"the library '{library}' has no license registered"))
    for library in sorted(set(licenses) - libraries):
        findings.append(
            Finding(NAME, THIRD_PARTY_FILE, f"the library '{library}' is registered but is not in vcpkg.json")
        )


def _check_retired_folders(root: Path, findings: list[Finding]) -> None:
    # The names are compared exactly: on Windows, "tools" would also find the "Tools" folder.
    folders = {entry.name for entry in root.iterdir() if entry.is_dir()}
    for folder in sorted(folders & _RETIRED_FOLDERS.keys()):
        findings.append(Finding(NAME, folder, f"this folder is not used: {_RETIRED_FOLDERS[folder]}"))


def _has_exact_folder(folder: Path, *names: str) -> bool:
    """Compares the exact names, because Windows would accept a folder written with the wrong case."""
    for name in names:
        if not folder.is_dir() or name not in {entry.name for entry in folder.iterdir() if entry.is_dir()}:
            return False
        folder = folder / name
    return True


def include_root(parent: str, module: str) -> tuple[str, ...]:
    """The folder of the public headers below Public/: Nexum/<Module> in the engine and the kit."""
    where = f"{parent}/{module}"
    if where in _INCLUDE_ROOTS:
        return _INCLUDE_ROOTS[where]
    if parent in ("Engine", "Kit"):
        return ("Nexum", module)
    return (module,)


def _top_level(root: Path) -> str:
    return read_text(root / "CMakeLists.txt") if (root / "CMakeLists.txt").is_file() else ""


def _modules(root: Path, parent: str) -> Iterator[Path]:
    folder = root / parent
    if folder.is_dir():
        yield from sorted(entry for entry in folder.iterdir() if entry.is_dir())


def _check_modules(root: Path, findings: list[Finding]) -> None:
    top_level = _top_level(root)
    for parent in _MODULE_FOLDERS:
        for module in _modules(root, parent):
            _check_module(parent, module, top_level, findings, needs_public=True)


def _check_tools(root: Path, findings: list[Finding]) -> None:
    top_level = _top_level(root)
    for module in _modules(root, _TOOL_FOLDER):
        _check_module(_TOOL_FOLDER, module, top_level, findings, needs_public=False)


def _check_module(parent: str, module: Path, top_level: str, findings: list[Finding], needs_public: bool) -> None:
    where = f"{parent}/{module.name}"
    if not _MODULE_NAME.match(module.name):
        findings.append(Finding(NAME, where, "modules must be named in PascalCase"))
        return
    if not (module / "CMakeLists.txt").is_file():
        findings.append(Finding(NAME, where, "the module needs a CMakeLists.txt"))
    if not (module / "Private").is_dir():
        findings.append(Finding(NAME, where, "the module needs a Private folder"))
    headers = include_root(parent, module.name)
    if needs_public and not _has_exact_folder(module, "Public", *headers):
        findings.append(Finding(NAME, where, f"public headers must live in Public/{'/'.join(headers)}"))
    if f"add_subdirectory({where})" not in top_level:
        findings.append(Finding(NAME, where, "the module is not added to the top-level CMakeLists.txt"))
    if not any(iter_files(module / "Tests", "Test.cpp")):
        findings.append(Finding(NAME, where, "the module needs tests in its Tests folder"))


def _check_file_names(root: Path, findings: list[Finding]) -> None:
    for folder in _CODE_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h", ".cc", ".cxx"):
            if not _CPP_NAME.match(file.name):
                findings.append(Finding(NAME, relative(root, file), "C++ files must be PascalCase, with .cpp or .hpp"))


def _test_folders(root: Path) -> Iterator[Path]:
    for folder in _PRODUCT_FOLDERS:
        if (root / folder).is_dir():
            yield from sorted(path for path in (root / folder).rglob("Tests") if path.is_dir())


def _check_test_folders(root: Path, findings: list[Finding]) -> None:
    for tests in _test_folders(root):
        owner = tests.parent / "CMakeLists.txt"
        if not owner.is_file() or "add_subdirectory(Tests)" not in read_text(owner):
            findings.append(Finding(NAME, relative(root, tests), "the tests are not added to the build of the module"))
        for file in iter_files(tests, ".cpp"):
            if not file.name.endswith("Test.cpp"):
                findings.append(Finding(NAME, relative(root, file), "test files must end with Test.cpp"))
