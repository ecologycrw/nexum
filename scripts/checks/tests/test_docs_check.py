import unittest

from support import TempRepositoryTest

from nexum_checks import docs_check

STATUS = "---\nfase: preparacao\nspec-atual:\n---\n\n# Status\n"
ADR = "---\nid: {id}\ntitulo: A decision\nstatus: {status}\nsubstituida-por: {by}\n---\n\n# ADR\n"
VISUAL_PROOF = "\n## Prova visual\n\nNão se aplica.\n"
CENTRAL_SOURCES = "## Fontes da central usadas\n\nNenhuma: a fatia não implementa comportamento do MU.\n"
SUFFICIENCY = "\n## Suficiência das descrições\n\nNão se aplica.\n"
PLAN = f"# Plan\n\n{CENTRAL_SOURCES}{SUFFICIENCY}"
CHECKED_PLAN = (
    "# Plan\n\n## Fontes da central usadas\n\n| Andar | spec, seção I.13.8 | b53c704c6 |\n"
    "\n## Suficiência das descrições\n\n### Andar\n\n| Item | O que a descrição diz |\n|---|---|\n"
    "| Ordem dos passos | um passo por vez |\n\n{questions}\n\n## Riscos\n\nNenhum.\n"
)
CLEAN_ROOM = "\n## Sala limpa\n\nNão se aplica.\n"
LOCKS_AND_TESTS = "\n## Travas alteradas e testes removidos\n\n- Travas alteradas: nenhuma.\n- Testes removidos: nenhum.\n"


class DocsCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        self.write("docs/STATUS.md", STATUS)

    def spec(self, status: str, criteria: str = "- CA-01: Something works.\n", extra: tuple[str, ...] = (),
             visual_proof: str = VISUAL_PROOF, plan: str = PLAN) -> None:
        self.write("specs/0001-first/spec.md",
                   f"---\nid: 0001\ntitulo: First\nstatus: {status}\n---\n\n{criteria}{visual_proof}")
        self.write("specs/0001-first/plan.md", plan)
        for name in extra:
            self.write(f"specs/0001-first/{name}", "# File\n")

    def report(self, text: str, visual_proof: str = VISUAL_PROOF, clean_room: str = CLEAN_ROOM,
               locks_and_tests: str = LOCKS_AND_TESTS) -> None:
        self.write("specs/0001-first/report.md", f"{text}{visual_proof}{clean_room}{locks_and_tests}")

    def adr(self, number: str, status: str = "aceita", by: str = "") -> None:
        name = f"{number}-decision.md"
        self.write(f"docs/adr/{name}", ADR.format(id=number, status=status, by=by))
        index = self.root / "docs/adr/README.md"
        previous = index.read_text(encoding="utf-8") if index.exists() else ""
        self.write("docs/adr/README.md", f"{previous}[{number}]({name})\n")

    def run_check(self) -> list:
        return docs_check.run(self.root)

    def test_a_valid_repository_has_no_findings(self) -> None:
        self.assertEqual(self.run_check(), [])

    def test_a_broken_link_is_refused(self) -> None:
        self.write("README.md", "[a](docs/missing.md)\n")
        self.assert_refused(self.run_check(), "broken link")

    def test_a_link_outside_the_repository_is_refused(self) -> None:
        self.write("README.md", "[a](../../secret.md)\n")
        self.assert_refused(self.run_check(), "outside the repository")

    def test_a_broken_anchor_is_refused(self) -> None:
        self.write("docs/A.md", "# Real heading\n")
        self.write("README.md", "[a](docs/A.md#missing)\n")
        self.assert_refused(self.run_check(), "anchor '#missing'")

    def test_an_anchor_with_accents_and_dashes_is_accepted(self) -> None:
        self.write("docs/A.md", "## Conteúdo de terceiros\n## D-01 — Caminho B: engine do zero, em sala limpa\n")
        self.write("README.md", "[a](docs/A.md#conteúdo-de-terceiros) [b](docs/A.md#d-01--caminho-b-engine-do-zero-em-sala-limpa)\n")
        self.assertEqual(self.run_check(), [])

    def test_links_inside_code_are_ignored(self) -> None:
        self.write("README.md", "`[a](missing.md)`\n\n```text\n[b](missing.md)\n```\n")
        self.assertEqual(self.run_check(), [])

    def test_external_links_are_ignored(self) -> None:
        self.write("README.md", "[a](https://example.com/x.md) [b](mailto:a@example.com)\n")
        self.assertEqual(self.run_check(), [])

    def test_the_copy_of_the_reference_spec_keeps_its_links_to_the_knowledge_center(self) -> None:
        self.write("docs/referencia/spec.md", "[a](../../../knowledge/x.md) [b](../../GLOSSARIO.md) [c](other.md)\n")
        self.assertEqual(self.run_check(), [])

    def test_links_into_the_copy_are_still_checked(self) -> None:
        self.write("docs/referencia/spec.md", "## Real heading\n")
        self.write("docs/A.md", "[a](referencia/spec.md#missing)\n")
        self.assert_refused(self.run_check(), "anchor '#missing'")

    def test_the_index_of_the_reference_folder_is_checked(self) -> None:
        self.write("docs/referencia/README.md", "[a](missing.md)\n")
        self.assert_refused(self.run_check(), "broken link")

    def test_a_spec_folder_needs_a_valid_name(self) -> None:
        self.write("specs/first/spec.md", "")
        self.assert_refused(self.run_check(), "NNNN-name-in-kebab-case")

    def test_a_spec_needs_a_known_status(self) -> None:
        self.spec("finished")
        self.assert_refused(self.run_check(), "front matter 'status'")

    def test_a_spec_id_must_match_the_folder(self) -> None:
        self.write("specs/0002-second/spec.md", "---\nid: 0001\ntitulo: A\nstatus: rascunho\n---\n")
        self.write("specs/0002-second/plan.md", "")
        self.assert_refused(self.run_check(), "'id' must be 0002")

    def test_an_approved_spec_needs_a_plan(self) -> None:
        self.write("specs/0001-first/spec.md", "---\nid: 0001\ntitulo: A\nstatus: aprovada\n---\n\n- CA-01: Works.\n")
        self.assert_refused(self.run_check(), "requires plan.md")

    def test_an_approved_spec_needs_criteria(self) -> None:
        self.spec("aprovada", criteria="No criteria here.\n")
        self.assert_refused(self.run_check(), "at least one acceptance criterion")

    def test_a_criterion_cannot_be_defined_twice(self) -> None:
        self.spec("aprovada", criteria="- CA-01: One.\n- CA-01: Two.\n")
        self.assert_refused(self.run_check(), "more than once")

    def test_a_spec_in_progress_needs_tasks(self) -> None:
        self.spec("em-andamento")
        self.assert_refused(self.run_check(), "requires tasks.md")

    def test_a_finished_spec_needs_a_report(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.assert_refused(self.run_check(), "requires report.md")

    def test_every_criterion_must_be_mentioned_in_the_report(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("Nothing here.\n")
        self.assert_refused(self.run_check(), "not mentioned in the report")

    def test_a_criterion_without_proof_is_refused(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | trust me |\n")
        self.assert_refused(self.run_check(), "has no proof")

    def test_a_tagged_test_of_any_layer_is_a_proof(self) -> None:
        for path in ("Engine/Core/Tests/ATest.cpp", "Kit/GridRpg/Tests/ATest.cpp", "Games/MuOnline/Tests/ATest.cpp",
                     "Tools/MuImport/Tests/ATest.cpp", "tests/architecture/ATest.cpp"):
            with self.subTest(path=path):
                self.spec("concluida", extra=("tasks.md",))
                self.report("| CA-01 | test |\n")
                self.write(path, 'TEST_CASE("[0001-CA-01] it works") {}\n')
                self.assertEqual(self.run_check(), [])
                (self.root / path).unlink()

    def test_a_tag_outside_the_tests_is_not_a_proof(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | test |\n")
        self.write("Engine/Core/Private/A.cpp", "// [0001-CA-01]\n")
        self.assert_refused(self.run_check(), "has no proof")

    def test_a_test_of_another_slice_is_not_a_proof(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | test |\n")
        self.write("Engine/Core/Tests/ATest.cpp", 'TEST_CASE("[0002-CA-01] it works") {}\n')
        self.assert_refused(self.run_check(), "has no proof")

    def test_a_manual_verification_is_a_proof(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n")
        self.assertEqual(self.run_check(), [])

    def test_a_draft_spec_can_leave_the_visual_proof_and_the_sources_for_later(self) -> None:
        self.spec("rascunho", visual_proof="", plan="# Plan\n")
        self.assertEqual(self.run_check(), [])
        self.spec("rascunho", plan=f"# Plan\n\n{CENTRAL_SOURCES}")
        self.assertEqual(self.run_check(), [])

    def test_an_approved_spec_needs_the_visual_proof_section(self) -> None:
        for status in ("aprovada", "em-andamento", "concluida"):
            with self.subTest(status=status):
                self.spec(status, extra=("tasks.md",), visual_proof="")
                self.assert_refused(self.run_check(), "spec.md: needs a '## Prova visual' section")

    def test_an_empty_visual_proof_section_is_refused(self) -> None:
        self.spec("aprovada", visual_proof="\n## Prova visual\n\n## Como o dono testa\n\nPassos.\n")
        self.assert_refused(self.run_check(), "needs a '## Prova visual' section")

    def test_a_section_may_be_split_into_subsections(self) -> None:
        self.spec("aprovada", visual_proof="\n## Prova visual\n\n### Cena\n\nO chão vazio.\n\n## Como o dono testa\n\nPassos.\n")
        self.assertEqual(self.run_check(), [])

    def test_a_section_with_only_empty_subsections_is_refused(self) -> None:
        self.spec("aprovada", visual_proof="\n## Prova visual\n\n### Cena\n\n## Como o dono testa\n\nPassos.\n")
        self.assert_refused(self.run_check(), "needs a '## Prova visual' section")

    def test_a_visual_proof_section_inside_a_code_block_does_not_count(self) -> None:
        self.spec("aprovada", visual_proof="\n```text\n## Prova visual\n\nNão se aplica.\n```\n")
        self.assert_refused(self.run_check(), "needs a '## Prova visual' section")

    def test_an_approved_plan_needs_the_sources_of_the_knowledge_center(self) -> None:
        for plan in (f"# Plan\n{SUFFICIENCY}", f"# Plan\n\n## Fontes da central usadas\n{SUFFICIENCY}"):
            with self.subTest(plan=plan):
                self.spec("aprovada", plan=plan)
                self.assert_refused(self.run_check(), "plan.md: needs a '## Fontes da central usadas' section")

    def test_an_approved_plan_needs_the_check_of_the_descriptions(self) -> None:
        for plan in (f"# Plan\n\n{CENTRAL_SOURCES}",
                     f"# Plan\n\n{CENTRAL_SOURCES}\n## Suficiência das descrições\n\n## Riscos\n\nNone.\n"):
            with self.subTest(plan=plan):
                self.spec("aprovada", plan=plan)
                self.assert_refused(self.run_check(), "plan.md: needs a '## Suficiência das descrições' section")

    def test_an_approved_plan_cannot_keep_open_questions_for_the_knowledge_center(self) -> None:
        for questions in ("Perguntas abertas para a central: em que ordem os passes são desenhados?",
                          "Perguntas abertas para a central:\n\n- em que ordem os passes são desenhados?"):
            with self.subTest(questions=questions):
                self.spec("aprovada", plan=CHECKED_PLAN.format(questions=questions))
                self.assert_refused(self.run_check(), "still has open questions for the knowledge center")

    def test_a_plan_without_open_questions_is_accepted(self) -> None:
        for answer in ("nenhuma", "Nenhuma.", "nenhuma"):
            with self.subTest(answer=answer):
                self.spec("aprovada", plan=CHECKED_PLAN.format(questions=f"Perguntas abertas para a central: {answer}"))
                self.assertEqual(self.run_check(), [])

    def test_a_draft_plan_can_keep_open_questions(self) -> None:
        self.spec("rascunho", plan=CHECKED_PLAN.format(questions="Perguntas abertas para a central: qual a ordem?"))
        self.assertEqual(self.run_check(), [])

    def test_open_questions_inside_a_code_block_are_only_an_example(self) -> None:
        questions = "Perguntas abertas para a central: nenhuma\n\n```text\nPerguntas abertas para a central: x?\n```"
        self.spec("aprovada", plan=CHECKED_PLAN.format(questions=questions))
        self.assertEqual(self.run_check(), [])

    def test_a_finished_report_needs_the_visual_proof_section(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n", visual_proof="")
        self.assert_refused(self.run_check(), "report.md: needs a '## Prova visual' section")

    def test_a_finished_report_needs_the_clean_room_section(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n", clean_room="")
        self.assert_refused(self.run_check(), "report.md: needs a '## Sala limpa' section")

    def test_a_finished_report_needs_the_locks_and_the_removed_tests(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n", locks_and_tests="")
        self.assert_refused(self.run_check(), "report.md: needs a '## Travas alteradas e testes removidos' section")

    def test_an_empty_section_of_locks_and_tests_is_refused(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n",
                    locks_and_tests="\n## Travas alteradas e testes removidos\n\n## Outra\n\nTexto.\n")
        self.assert_refused(self.run_check(), "Travas alteradas e testes removidos")

    def test_an_older_report_may_keep_the_locks_and_the_tests_apart(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        apart = "\n## Travas alteradas\n\nNenhuma.\n\n## Testes removidos\n\nNenhum.\n"
        self.report("| CA-01 | verificação manual: abri a janela |\n", locks_and_tests=apart)
        self.assertEqual(self.run_check(), [])

    def test_an_older_report_needs_both_sections(self) -> None:
        self.spec("concluida", extra=("tasks.md",))
        self.report("| CA-01 | verificação manual: abri a janela |\n", locks_and_tests="\n## Travas alteradas\n\nNenhuma.\n")
        self.assert_refused(self.run_check(), "Travas alteradas e testes removidos")

    def test_status_needs_a_phase(self) -> None:
        self.write("docs/STATUS.md", "---\nspec-atual:\n---\n")
        self.assert_refused(self.run_check(), "'fase' is missing")

    def test_status_must_point_to_an_existing_spec(self) -> None:
        self.write("docs/STATUS.md", "---\nfase: fatia\nspec-atual: 0007\n---\n")
        self.assert_refused(self.run_check(), "no specs/0007-* folder")

    def test_adr_numbers_cannot_have_gaps(self) -> None:
        self.adr("0001")
        self.adr("0003")
        self.assert_refused(self.run_check(), "expected 0002")

    def test_adr_needs_a_known_status(self) -> None:
        self.adr("0001", status="maybe")
        self.assert_refused(self.run_check(), "front matter 'status'")

    def test_a_replaced_adr_must_point_to_its_replacement(self) -> None:
        self.adr("0001", status="substituida", by="0009")
        self.assert_refused(self.run_check(), "substituida-por")

    def test_the_adr_index_must_list_every_adr(self) -> None:
        self.adr("0001")
        self.write("docs/adr/README.md", "empty\n")
        self.assert_refused(self.run_check(), "does not link to")

    def test_adr_file_names_must_be_ascii(self) -> None:
        self.write("docs/adr/0001-áudio.md", ADR.format(id="0001", status="aceita", by=""))
        self.assert_refused(self.run_check(), "NNNN-name-in-kebab-case.md")

    def debt(self, destination: str) -> None:
        self.write("docs/TECH_DEBT.md", f"| ID | Item | Destino |\n|---|---|---|\n| TD-0001 | Thing | {destination} |\n")

    def test_a_provisional_item_needs_a_destination(self) -> None:
        for destination in ("algum dia", "", "marco"):
            with self.subTest(destination=destination):
                self.debt(destination)
                self.assert_refused(self.run_check(), "needs a destination")

    def test_a_provisional_item_can_point_to_a_slice_or_a_milestone(self) -> None:
        for destination in ("fatia 3", "fatia de escala", "M0", "M12"):
            with self.subTest(destination=destination):
                self.debt(destination)
                self.assertEqual(self.run_check(), [])

    def test_a_proposal_needs_a_status(self) -> None:
        self.write("docs/PROPOSALS.md", "## Abertas\n\n### P-0001: Idea\n- Origem: owner\n")
        self.assert_refused(self.run_check(), "needs a '- Status:'")

    def test_a_proposal_number_cannot_repeat(self) -> None:
        entry = "### P-0001: Idea\n- Status: aberta\n\n"
        self.write("docs/PROPOSALS.md", entry + entry)
        self.assert_refused(self.run_check(), "used more than once")


if __name__ == "__main__":
    unittest.main()
