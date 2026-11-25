"""Keeps the documentation aligned: links, specs, STATUS, ADRs, TECH_DEBT and PROPOSALS.

The rules come from docs/WORKFLOW.md and docs/adr/0026-documentacao-como-memoria.md. The clean-room sections
("Fontes da central usadas" in the plan, "Sala limpa" in the report) come from docs/adr/0027-*.md.
"""

from __future__ import annotations

import itertools
import re
from pathlib import Path
from urllib.parse import unquote

from . import markdown
from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "docs"
SPEC_STATUSES = ("rascunho", "aprovada", "em-andamento", "concluida")
ADR_STATUSES = ("proposta", "aceita", "substituida", "rejeitada")
PROPOSAL_STATUSES = ("aberta", "aceita", "descartada")

_SPEC_FOLDER = re.compile(r"^\d{4}-[a-z0-9]+(?:-[a-z0-9]+)*$")
_SPEC_NUMBER = re.compile(r"^\d{4}$")
_CRITERION = re.compile(r"^\s*[-*]\s+\**(?P<id>CA-\d{2})\**\s*:")
_ADR_FILE = re.compile(r"^(?P<number>\d{4})-[a-z0-9]+(?:-[a-z0-9]+)*\.md$")
_DEBT_DESTINATION = re.compile(r"^(fatia (\d+|de [\w ]+|da [\w ]+|do [\w ]+)|M\d+)$", re.IGNORECASE)
_PROPOSAL_HEADING = re.compile(r"^###\s+(?P<id>P-\d{4})\b")
_PROPOSAL_STATUS = re.compile(r"^\s*[-*]\s+Status:\s*(?P<value>[\w-]+)")
_VISUAL_PROOF = "Prova visual"
_VISUAL_PROOF_MISSING = "needs a '## Prova visual' section: describe the proof or write that it does not apply"
_CENTRAL_SOURCES = "Fontes da central usadas"
_CENTRAL_SOURCES_MISSING = (
    "needs a '## Fontes da central usadas' section: list the specs and findings used, or write that the slice "
    "implements no behavior of the original (INV-16)"
)
_SUFFICIENCY = "Suficiência das descrições"
_SUFFICIENCY_MISSING = (
    "needs a '## Suficiência das descrições' section: check that each description of the knowledge center says "
    "everything the code and the tests need, or write that it does not apply (INV-16)"
)
_OPEN_QUESTIONS = re.compile(r"^\s*Perguntas abertas para a central:\s*(?P<answer>.*?)\s*$")
_NO_OPEN_QUESTIONS = re.compile(r"^nenhuma\.?$", re.IGNORECASE)
_CLEAN_ROOM = "Sala limpa"
_SECTION_END = re.compile(r"^\s{0,3}#{1,2}\s")
_ANY_HEADING = re.compile(r"^\s{0,3}#{1,6}\s")
_CLEAN_ROOM_MISSING = (
    "needs a '## Sala limpa' section with the clean-room checklist, or write that it does not apply (INV-16)"
)
_LOCKS_AND_TESTS = "Travas alteradas e testes removidos"
# Reports written before the template joined the two sections (slice 0) keep them apart; they are historical.
_LOCKS_AND_TESTS_APART = ("Travas alteradas", "Testes removidos")
_LOCKS_AND_TESTS_MISSING = (
    "needs a '## Travas alteradas e testes removidos' section: list the locks that changed and the tests removed or "
    "weakened, with the reason, or write 'nenhuma' and 'nenhum'"
)
# Faithful copies of documents that live in the knowledge center: their links point there (docs/referencia/README.md).
_EXTERNAL_COPIES = "docs/referencia/"
_EXTERNAL_COPIES_INDEX = "docs/referencia/README.md"


def is_external_copy(where: str) -> bool:
    return where.startswith(_EXTERNAL_COPIES) and where != _EXTERNAL_COPIES_INDEX


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    _check_links(root, findings)
    _check_specs(root, findings)
    _check_status(root, findings)
    _check_adrs(root, findings)
    _check_tech_debt(root, findings)
    _check_proposals(root, findings)
    return findings


def _is_inside(root: Path, path: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
    except ValueError:
        return False
    return True


def _check_links(root: Path, findings: list[Finding]) -> None:
    for file in iter_files(root, ".md"):
        where = relative(root, file)
        if is_external_copy(where):
            continue
        for target, line in markdown.links(read_text(file)):
            if "://" in target or target.startswith("mailto:"):
                continue
            path_part, _, anchor = target.partition("#")
            resolved = file
            if path_part:
                decoded = unquote(path_part)
                resolved = root / decoded.lstrip("/") if decoded.startswith("/") else file.parent / decoded
            if not _is_inside(root, resolved):
                findings.append(Finding(NAME, where, f"line {line}: link '{target}' points outside the repository"))
            elif not resolved.exists():
                findings.append(Finding(NAME, where, f"line {line}: broken link '{target}'"))
            elif anchor and resolved.is_file() and resolved.suffix == ".md":
                if unquote(anchor) not in markdown.anchors(read_text(resolved)):
                    findings.append(
                        Finding(NAME, where, f"line {line}: anchor '#{anchor}' does not exist in "
                                             f"'{relative(root, resolved)}'")
                    )


def _required_spec_files(status: str) -> tuple[str, ...]:
    if status == "em-andamento":
        return ("spec.md", "plan.md", "tasks.md")
    if status == "concluida":
        return ("spec.md", "plan.md", "tasks.md", "report.md")
    return ("spec.md", "plan.md")


def _is_test_source(root: Path, file: Path) -> bool:
    parts = file.relative_to(root).parts
    return parts[0] == "tests" or "Tests" in parts


def _test_sources(root: Path) -> str:
    return "\n".join(read_text(file) for file in iter_files(root, ".cpp") if _is_test_source(root, file))


def _check_specs(root: Path, findings: list[Finding]) -> None:
    specs = root / "specs"
    if not specs.is_dir():
        return
    for folder in sorted(entry for entry in specs.iterdir() if entry.is_dir()):
        _check_spec_folder(root, folder, findings)


def _check_spec_folder(root: Path, folder: Path, findings: list[Finding]) -> None:
    where = f"specs/{folder.name}"
    if not _SPEC_FOLDER.match(folder.name):
        findings.append(Finding(NAME, where, "spec folders must be named NNNN-name-in-kebab-case"))
        return
    spec_file = folder / "spec.md"
    if not spec_file.is_file():
        findings.append(Finding(NAME, where, "spec.md is missing"))
        return

    number = folder.name[:4]
    spec_where = f"{where}/spec.md"
    text = read_text(spec_file)
    front_matter = markdown.read_front_matter(text)
    if front_matter.get("id") != number:
        findings.append(Finding(NAME, spec_where, f"front matter 'id' must be {number}"))
    if not front_matter.get("titulo"):
        findings.append(Finding(NAME, spec_where, "front matter 'titulo' is missing"))

    status = front_matter.get("status", "")
    if status not in SPEC_STATUSES:
        findings.append(Finding(NAME, spec_where, f"front matter 'status' must be one of: {', '.join(SPEC_STATUSES)}"))
        return
    for required in _required_spec_files(status):
        if not (folder / required).is_file():
            findings.append(Finding(NAME, where, f"status '{status}' requires {required}"))

    criteria = [match.group("id") for line in markdown.prose_lines(text) if (match := _CRITERION.match(line))]
    for duplicate in sorted({item for item in criteria if criteria.count(item) > 1}):
        findings.append(Finding(NAME, spec_where, f"acceptance criterion {duplicate} is defined more than once"))
    if status != "rascunho" and not criteria:
        findings.append(Finding(NAME, spec_where, "an approved spec needs at least one acceptance criterion (- CA-01: ...)"))
    if status != "rascunho" and not has_section(text, _VISUAL_PROOF):
        findings.append(Finding(NAME, spec_where, _VISUAL_PROOF_MISSING))

    plan = folder / "plan.md"
    if status != "rascunho" and plan.is_file():
        _check_plan(f"{where}/plan.md", read_text(plan), findings)

    report = folder / "report.md"
    if status == "concluida" and report.is_file():
        report_text = read_text(report)
        _check_criteria_proof(root, number, f"{where}/report.md", sorted(set(criteria)), report_text, findings)
        if not has_section(report_text, _VISUAL_PROOF):
            findings.append(Finding(NAME, f"{where}/report.md", _VISUAL_PROOF_MISSING))
        if not has_section(report_text, _CLEAN_ROOM):
            findings.append(Finding(NAME, f"{where}/report.md", _CLEAN_ROOM_MISSING))
        if not has_locks_and_tests(report_text):
            findings.append(Finding(NAME, f"{where}/report.md", _LOCKS_AND_TESTS_MISSING))


def _check_plan(where: str, text: str, findings: list[Finding]) -> None:
    """The clean room of an approved plan: the sources, the check of each description, and no open question."""
    if not has_section(text, _CENTRAL_SOURCES):
        findings.append(Finding(NAME, where, _CENTRAL_SOURCES_MISSING))
    if not has_section(text, _SUFFICIENCY):
        findings.append(Finding(NAME, where, _SUFFICIENCY_MISSING))
    for number, line in enumerate(markdown.prose_lines(text), start=1):
        match = _OPEN_QUESTIONS.match(line)
        if match and not _NO_OPEN_QUESTIONS.match(match.group("answer")):
            findings.append(Finding(
                NAME, where,
                f"line {number}: an approved plan still has open questions for the knowledge center: ask them "
                "before the approval, or write 'Perguntas abertas para a central: nenhuma'",
            ))


def has_section(text: str, title: str) -> bool:
    """The '## title' section must exist and say something, even if only that it does not apply."""
    heading = re.compile(rf"^##\s+{re.escape(title)}\s*$")
    lines = markdown.prose_lines(text)
    start = next((index for index, line in enumerate(lines) if heading.match(line)), None)
    if start is None:
        return False
    # The section ends at the next heading of the same level or above; its subsections are part of it.
    body = itertools.takewhile(lambda line: not _SECTION_END.match(line), lines[start + 1:])
    return any(line.strip() and not _ANY_HEADING.match(line) for line in body)


def has_locks_and_tests(report: str) -> bool:
    """The report says which locks changed and which tests were removed, so neither goes unnoticed."""
    return has_section(report, _LOCKS_AND_TESTS) or all(has_section(report, title) for title in _LOCKS_AND_TESTS_APART)


def _check_criteria_proof(
    root: Path, number: str, where: str, criteria: list[str], report: str, findings: list[Finding]
) -> None:
    lines = markdown.split_lines(report)
    sources = _test_sources(root)
    for criterion in criteria:
        mentions = [line for line in lines if criterion in line]
        if not mentions:
            findings.append(Finding(NAME, where, f"{criterion} is not mentioned in the report"))
            continue
        tag = f"[{number}-{criterion}]"
        has_manual_check = any("manual" in line.lower() for line in mentions)
        if tag not in sources and not has_manual_check:
            findings.append(
                Finding(NAME, where, f"{criterion} has no proof: no test tagged {tag} and no manual verification in the report")
            )


def _check_status(root: Path, findings: list[Finding]) -> None:
    where = "docs/STATUS.md"
    file = root / "docs" / "STATUS.md"
    if not file.is_file():
        findings.append(Finding(NAME, where, "the file is missing"))
        return
    front_matter = markdown.read_front_matter(read_text(file))
    if not front_matter.get("fase"):
        findings.append(Finding(NAME, where, "front matter 'fase' is missing"))
    spec = front_matter.get("spec-atual", "")
    if not spec:
        return
    if not _SPEC_NUMBER.match(spec):
        findings.append(Finding(NAME, where, "'spec-atual' must be a four-digit number or empty"))
    elif not list((root / "specs").glob(f"{spec}-*")):
        findings.append(Finding(NAME, where, f"'spec-atual' is {spec}, but there is no specs/{spec}-* folder"))


def _check_adrs(root: Path, findings: list[Finding]) -> None:
    folder = root / "docs" / "adr"
    if not folder.is_dir():
        return
    adrs: dict[int, str] = {}
    for file in sorted(folder.glob("*.md")):
        if file.name == "README.md":
            continue
        match = _ADR_FILE.match(file.name)
        if not match:
            findings.append(Finding(NAME, f"docs/adr/{file.name}", "ADR files must be named NNNN-name-in-kebab-case.md"))
        elif int(match.group("number")) in adrs:
            findings.append(Finding(NAME, f"docs/adr/{file.name}", "another ADR already uses this number"))
        else:
            adrs[int(match.group("number"))] = file.name

    expected = 1
    for number in sorted(adrs):
        name = adrs[number]
        if number != expected:
            findings.append(Finding(NAME, f"docs/adr/{name}", f"ADR numbers must have no gaps: expected {expected:04d}"))
        expected = number + 1
        _check_adr_front_matter(folder / name, adrs, findings)

    index_file = folder / "README.md"
    index = read_text(index_file) if index_file.is_file() else ""
    for name in adrs.values():
        if f"({name})" not in index:
            findings.append(Finding(NAME, "docs/adr/README.md", f"the ADR index does not link to {name}"))


def _check_adr_front_matter(file: Path, adrs: dict[int, str], findings: list[Finding]) -> None:
    where = f"docs/adr/{file.name}"
    front_matter = markdown.read_front_matter(read_text(file))
    if front_matter.get("id") != file.name[:4]:
        findings.append(Finding(NAME, where, f"front matter 'id' must be {file.name[:4]}"))
    if not front_matter.get("titulo"):
        findings.append(Finding(NAME, where, "front matter 'titulo' is missing"))
    status = front_matter.get("status", "")
    if status not in ADR_STATUSES:
        findings.append(Finding(NAME, where, f"front matter 'status' must be one of: {', '.join(ADR_STATUSES)}"))
    elif status == "substituida":
        replacement = front_matter.get("substituida-por", "")
        if not replacement.isdigit() or int(replacement) not in adrs:
            findings.append(Finding(NAME, where, "a replaced ADR needs 'substituida-por' with the number of an existing ADR"))


def _check_tech_debt(root: Path, findings: list[Finding]) -> None:
    where = "docs/TECH_DEBT.md"
    file = root / "docs" / "TECH_DEBT.md"
    if not file.is_file():
        return
    column: int | None = None
    row = 0
    for number, line in enumerate(markdown.prose_lines(read_text(file)), start=1):
        trimmed = line.strip()
        if not trimmed.startswith("|"):
            column = None
            row = 0
            continue
        row += 1
        cells = [cell.strip() for cell in trimmed.strip("|").split("|")]
        if row == 1:
            lowered = [cell.lower() for cell in cells]
            column = lowered.index("destino") if "destino" in lowered else None
        elif row > 2 and column is not None:
            destination = cells[column] if column < len(cells) else ""
            if not _DEBT_DESTINATION.match(destination):
                findings.append(
                    Finding(
                        NAME, where,
                        f"line {number}: every item needs a destination like 'fatia 3' or 'M2' (found '{destination}')",
                    )
                )


def _check_proposals(root: Path, findings: list[Finding]) -> None:
    where = "docs/PROPOSALS.md"
    file = root / "docs" / "PROPOSALS.md"
    if not file.is_file():
        return
    seen: set[str] = set()
    current: str | None = None
    has_status = False

    def close() -> None:
        if current is not None and not has_status:
            findings.append(Finding(NAME, where, f"{current} needs a '- Status:' line with one of: {', '.join(PROPOSAL_STATUSES)}"))

    for line in markdown.prose_lines(read_text(file)):
        heading = _PROPOSAL_HEADING.match(line)
        if heading:
            close()
            current = heading.group("id")
            has_status = False
            if current in seen:
                findings.append(Finding(NAME, where, f"{current} is used more than once"))
            seen.add(current)
        elif line.lstrip().startswith("#"):
            close()
            current = None
        else:
            status = _PROPOSAL_STATUS.match(line)
            if current is not None and status and status.group("value") in PROPOSAL_STATUSES:
                has_status = True
    close()
