import unittest

from support import TempRepositoryTest

from nexum_checks import terms_check


class TermsCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        self.write(
            terms_check.TERMS_FILE,
            "# comment\nacmegame\nacme town\nsecretformat ; forbidden: Engine/*, Kit/*, Games/*\n",
        )

    def test_a_term_of_the_game_is_refused_in_the_engine_and_in_the_kit(self) -> None:
        for path in ("Engine/Core/Private/A.cpp", "Kit/GridRpg/Public/Nexum/GridRpg/A.hpp", "Engine/Core/CMakeLists.txt",
                     "Kit/GridRpg/Tests/ATest.cpp", "tests/architecture/OtherGame/A.cpp"):
            with self.subTest(path=path):
                self.write(path, "// reads files of AcmeGame\n")
                self.assert_refused(terms_check.run(self.root), path)

    def test_a_term_of_the_game_is_allowed_in_the_game_module_the_tools_and_the_docs(self) -> None:
        for path in ("Games/AcmeGame/Private/A.cpp", "Tools/Importer/Private/A.cpp", "docs/VISION.md", "README.md",
                     "scripts/checks/a.py", "tests/hooks/a.py"):
            self.write(path, "AcmeGame and Acme Town\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_a_term_with_its_own_folders_is_refused_only_there(self) -> None:
        self.write("Games/AcmeGame/Private/A.cpp", "// secretformat\n")
        self.assert_refused(terms_check.run(self.root), "Games/AcmeGame/Private/A.cpp")

    def test_a_term_with_its_own_folders_is_allowed_everywhere_else(self) -> None:
        self.write("Tools/Importer/Private/A.cpp", "// reads secretformat\n")
        self.write("docs/VISION.md", "secretformat\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_terms_of_two_words_are_found(self) -> None:
        self.write("Engine/Core/Private/A.cpp", "// welcome to Acme  Town\n// welcome to acme town\n")
        self.assert_refused(terms_check.run(self.root), "line 2")

    def test_only_whole_words_count(self) -> None:
        self.write("Engine/Core/Private/A.cpp", "// acmegames and myacmegame are other words\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_binary_files_are_skipped(self) -> None:
        self.write("Engine/Core/Private/placeholder.txt", "")
        (self.root / "Engine" / "Core" / "asset.bin").write_bytes(b"\0acmegame")
        self.assertEqual(terms_check.run(self.root), [])

    def test_a_line_that_cannot_be_read_is_reported(self) -> None:
        self.write(terms_check.TERMS_FILE, "acmegame ; Engine/*\n")
        self.assert_refused(terms_check.run(self.root), "forbidden: folder")

    def test_a_missing_terms_file_is_reported(self) -> None:
        (self.root / terms_check.TERMS_FILE).unlink()
        self.assert_refused(terms_check.run(self.root), "the terms file is missing")


class RealTermsTests(TempRepositoryTest):
    """The real list of the repository: the red line of the Nexum (INV-03 and INV-08)."""

    def setUp(self) -> None:
        super().setUp()
        real = support_root() / terms_check.TERMS_FILE
        self.write(terms_check.TERMS_FILE, real.read_text(encoding="utf-8"))

    def test_the_namespace_and_the_maps_of_the_game_are_refused_in_the_engine(self) -> None:
        for code in ("namespace mu {}", "// Lorencia at noon", "// the Dark Knight", "auto key = 0xC3;"):
            with self.subTest(code=code):
                self.write("Engine/Render/Private/A.cpp", code + "\n")
                self.assert_refused(terms_check.run(self.root), "Engine/Render/Private/A.cpp")

    def test_words_that_only_contain_the_name_are_allowed(self) -> None:
        self.write("Engine/Core/Private/A.cpp", "std::mutex mutex; // mutable, multiply\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_the_original_formats_are_refused_in_the_game_module(self) -> None:
        self.write("Games/MuOnline/Private/A.cpp", "// opens the .bmd file\n")
        self.assert_refused(terms_check.run(self.root), "the term 'bmd'")

    def test_the_original_formats_are_allowed_in_the_tools(self) -> None:
        self.write("Tools/MuImport/Private/A.cpp", "// reads the .bmd and .ozj files of the user\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_the_game_module_may_name_the_game(self) -> None:
        self.write("Games/MuOnline/Private/A.cpp", "namespace mu { /* Lorencia */ }\n")
        self.assertEqual(terms_check.run(self.root), [])


def support_root():
    from pathlib import Path

    return Path(__file__).resolve().parents[3]


if __name__ == "__main__":
    unittest.main()
