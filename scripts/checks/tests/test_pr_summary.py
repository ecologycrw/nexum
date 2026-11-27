import unittest

import support  # noqa: F401

from nexum_checks import pr_summary


class PrSummaryTests(unittest.TestCase):
    def test_protected_paths_are_recognized(self) -> None:
        for path in (".claude/settings.json", ".github/workflows/ci.yml", "cmake/NexumModule.cmake", "AGENTS.md",
                     "scripts/dev.py", "scripts/checks/run.py", "scripts/checks/forbidden_terms.txt",
                     "tests/architecture/CMakeLists.txt", "docs/ROADMAP.md", "docs/VISUAL.md",
                     "tests/visual/approved/scene.png", "docs/referencia/especificacao-engine-propria-caminho-b.md"):
            with self.subTest(path=path):
                self.assertTrue(pr_summary.is_protected(path))

    def test_ordinary_paths_are_not_protected(self) -> None:
        for path in ("Engine/Core/Private/Version.cpp", "docs/STATUS.md", "Engine/Core/Tests/VersionTest.cpp",
                     "scripts/other.py"):
            with self.subTest(path=path):
                self.assertFalse(pr_summary.is_protected(path))

    def test_renames_list_both_paths(self) -> None:
        self.assertEqual(pr_summary.parse_name_status("R100\ta.md\tb.md\nM\tc.md\n"),
                         [("R", "a.md"), ("R", "b.md"), ("M", "c.md")])

    def test_a_clean_change_reports_no_protected_files(self) -> None:
        summary = pr_summary.build([("M", "Engine/Core/Private/A.cpp")], "")
        self.assertIn("Travas alteradas (0)", summary)
        self.assertIn("Nenhuma.", summary)

    def test_a_protected_change_is_highlighted(self) -> None:
        summary = pr_summary.build([("M", "AGENTS.md")], "")
        self.assertIn("`AGENTS.md` (alterado)", summary)
        self.assertIn("aprovação explícita do dono", summary)

    def test_added_and_removed_tests_are_counted(self) -> None:
        diff = '+TEST_CASE("a") {\n+SUBCASE("b") {\n-TEST_CASE_FIXTURE(F, "c") {\n+int other;\n'
        summary = pr_summary.build([], diff)
        self.assertIn("Testes adicionados: 2", summary)
        self.assertIn("Testes removidos: 1", summary)
        self.assertIn("testes foram removidos", summary)

    def test_deleted_test_files_are_listed(self) -> None:
        summary = pr_summary.build([("D", "tests/architecture/ATest.cpp"), ("D", "Engine/Core/Private/A.cpp")], "")
        self.assertIn("Arquivos de teste apagados: 1", summary)
        self.assertIn("`tests/architecture/ATest.cpp`", summary)

    def test_deleted_test_files_of_a_module_are_listed(self) -> None:
        for path in ("Engine/Core/Tests/ATest.cpp", "Kit/GridRpg/Tests/ATest.cpp", "Games/MuOnline/Tests/ATest.cpp"):
            with self.subTest(path=path):
                summary = pr_summary.build([("D", path)], "")
                self.assertIn("Arquivos de teste apagados: 1", summary)
                self.assertIn(f"`{path}`", summary)

    def test_removed_lines_of_the_powershell_tests_are_counted(self) -> None:
        diff = ("--- a/tests/hooks/guard-bash.tests.ps1\n+++ b/tests/hooks/guard-bash.tests.ps1\n"
                "-    'git push --force',\n-\n-# a comment\n+    'git push -f',\n"
                "--- a/tests/hooks/test_git_hooks.py\n+++ b/tests/hooks/test_git_hooks.py\n-x = 1\n")
        summary = pr_summary.build([], diff)
        self.assertIn("Linhas removidas dos testes em PowerShell (os casos dos hooks): 1", summary)
        self.assertIn("linhas saíram dos testes em PowerShell", summary)

    def test_a_change_without_powershell_removals_has_no_warning(self) -> None:
        summary = pr_summary.build([], "--- a/tests/hooks/a.tests.ps1\n+++ b/tests/hooks/a.tests.ps1\n+    'new case',\n")
        self.assertIn("(os casos dos hooks): 0", summary)
        self.assertNotIn("linhas saíram", summary)

    def test_deleted_powershell_test_files_are_listed(self) -> None:
        summary = pr_summary.build([("D", "tests/hooks/guard-bash.tests.ps1")], "")
        self.assertIn("Arquivos de teste apagados: 1", summary)

    def test_the_diff_of_tests_includes_the_tests_of_the_modules_and_of_the_checks(self) -> None:
        self.assertIn(":(glob)**/Tests/**", pr_summary.TEST_PATHSPECS)
        self.assertIn("scripts/checks/tests/", pr_summary.TEST_PATHSPECS)

    def test_new_nolints_are_listed_with_the_file_and_the_line(self) -> None:
        diff = (
            "diff --git a/Engine/Core/Private/A.cpp b/Engine/Core/Private/A.cpp\n"
            "--- a/Engine/Core/Private/A.cpp\n"
            "+++ b/Engine/Core/Private/A.cpp\n"
            "@@ -10,0 +11,2 @@ namespace {\n"
            "+// The reason.\n"
            "+// NOLINTNEXTLINE(misc-check)\n"
            "@@ -20 +22 @@\n"
            "-int a = 1; // NOLINT(misc-old)\n"
            "+int a = `2`; // NOLINT(misc-new)\n"
            "diff --git a/Tools/PacketGen/Private/template.py b/Tools/PacketGen/Private/template.py\n"
            "new file mode 100644\n"
            "--- /dev/null\n"
            "+++ b/Tools/PacketGen/Private/template.py\n"
            "@@ -0,0 +1,3 @@\n"
            "+HEADER = '''\n"
            "+// NOLINTBEGIN(readability-function-size)\n"
            "+'''\n"
        )
        summary = pr_summary.build([], "", diff)
        self.assertIn("### `NOLINT` novos (3)", summary)
        self.assertIn("- `Engine/Core/Private/A.cpp`, linha 12: `// NOLINTNEXTLINE(misc-check)`", summary)
        self.assertIn("- `Engine/Core/Private/A.cpp`, linha 22: `int a = '2'; // NOLINT(misc-new)`", summary)
        self.assertIn("- `Tools/PacketGen/Private/template.py`, linha 2: `// NOLINTBEGIN(readability-function-size)`",
                      summary)
        self.assertNotIn("misc-old", summary)
        self.assertIn("cada `NOLINT` novo desliga uma conferência", summary)

    def test_nolints_of_the_documents_and_of_removed_files_are_not_listed(self) -> None:
        diff = (
            "diff --git a/docs/A.md b/docs/A.md\n--- a/docs/A.md\n+++ b/docs/A.md\n@@ -1,0 +2 @@\n"
            "+Um `NOLINT` explicado.\n"
            "diff --git a/Engine/Core/Private/B.cpp b/Engine/Core/Private/B.cpp\ndeleted file mode 100644\n"
            "--- a/Engine/Core/Private/B.cpp\n+++ /dev/null\n@@ -1 +0,0 @@\n-int b; // NOLINT(misc-check)\n"
        )
        summary = pr_summary.build([], "", diff)
        self.assertIn("### `NOLINT` novos (0)", summary)
        self.assertIn("Nenhum.", summary)
        self.assertNotIn("desliga uma conferência", summary)

    def test_the_diff_of_the_code_leaves_out_only_the_documents(self) -> None:
        self.assertEqual(pr_summary.CODE_PATHSPECS, (".", ":(exclude)*.md"))


if __name__ == "__main__":
    unittest.main()
