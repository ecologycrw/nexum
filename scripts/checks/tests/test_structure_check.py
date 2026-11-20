import shutil
import unittest

from support import TempRepositoryTest

from nexum_checks import structure_check

CORE = "Engine/Core"


class StructureCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        for required in structure_check.REQUIRED_FILES:
            self.write(required, "")
        self.write("vcpkg.json", '{"dependencies": ["doctest", {"name": "sdl3"}]}')
        self.write(structure_check.THIRD_PARTY_FILE, "| `doctest` | MIT | tests |\n| `sdl3` | zlib | window |\n")
        self.write("CMakeLists.txt", f"add_subdirectory({CORE})\n")
        self.module(CORE, "Nexum/Core", "Version")

    def module(self, where: str, headers: str, name: str) -> None:
        self.write(f"{where}/CMakeLists.txt", "add_subdirectory(Tests)\n")
        self.write(f"{where}/Private/{name}.cpp", "")
        self.write(f"{where}/Public/{headers}/{name}.hpp", "")
        self.write(f"{where}/Tests/{name}Test.cpp", "")

    def add_to_build(self, *modules: str) -> None:
        self.write("CMakeLists.txt", "".join(f"add_subdirectory({module})\n" for module in (CORE, *modules)))

    def test_a_valid_repository_has_no_findings(self) -> None:
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_missing_required_file_is_refused(self) -> None:
        for required in ("docs/VISION.md", structure_check.REFERENCE_SPEC, "docs/referencia/README.md"):
            with self.subTest(required=required):
                (self.root / required).unlink()
                self.assert_refused(structure_check.run(self.root), required)
                self.write(required, "")

    def test_the_visual_guide_the_license_list_and_the_reference_spec_are_required(self) -> None:
        for required in ("docs/VISUAL.md", "docs/THIRD_PARTY.md", structure_check.REFERENCE_SPEC):
            with self.subTest(required=required):
                self.assertIn(required, structure_check.REQUIRED_FILES)

    def test_a_library_without_a_registered_license_is_refused(self) -> None:
        self.write(structure_check.THIRD_PARTY_FILE, "| `doctest` | MIT | tests |\n")
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' has no license registered")

    def test_a_library_with_an_empty_license_is_refused(self) -> None:
        self.write(structure_check.THIRD_PARTY_FILE, "| `doctest` | MIT | tests |\n| `sdl3` |  | window |\n")
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' has no license registered")

    def test_a_library_that_left_the_build_must_leave_the_list(self) -> None:
        self.write("vcpkg.json", '{"dependencies": ["doctest"]}')
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' is registered but is not in vcpkg.json")

    def test_planned_libraries_written_without_backticks_are_not_registrations(self) -> None:
        self.write(structure_check.THIRD_PARTY_FILE,
                   "| `doctest` | MIT | tests |\n| `sdl3` | zlib | window |\n\n- EnTT (MIT): later.\n")
        self.assertEqual(structure_check.run(self.root), [])

    def test_an_invalid_library_manifest_is_refused(self) -> None:
        self.write("vcpkg.json", "{")
        self.assert_refused(structure_check.run(self.root), "not valid JSON")

    def test_a_module_must_be_named_in_pascal_case(self) -> None:
        self.write("Engine/nexum_physics/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "modules must be named")

    def test_the_kit_keeps_its_headers_below_the_nexum_folder(self) -> None:
        self.module("Kit/GridRpg", "Nexum/GridRpg", "KitIdentity")
        self.add_to_build("Kit/GridRpg")
        self.assertEqual(structure_check.run(self.root), [])

    def test_the_kit_follows_the_same_module_rules(self) -> None:
        self.write("Kit/GridRpg/CMakeLists.txt", "")
        findings = structure_check.run(self.root)
        self.assert_refused(findings, "needs a Private folder")
        self.assert_refused(findings, "Public/Nexum/GridRpg")
        self.assert_refused(findings, "needs tests")

    def test_the_game_module_keeps_its_headers_in_the_mu_folder(self) -> None:
        self.module("Games/MuOnline", "Mu", "ModuleIdentity")
        self.add_to_build("Games/MuOnline")
        self.assertEqual(structure_check.run(self.root), [])

    def test_the_game_module_cannot_use_another_header_folder(self) -> None:
        self.module("Games/MuOnline", "MuOnline", "ModuleIdentity")
        self.add_to_build("Games/MuOnline")
        self.assert_refused(structure_check.run(self.root), "Public/Mu")

    def test_a_tool_needs_private_code_and_tests_but_no_public_headers(self) -> None:
        self.write("Tools/MuImport/CMakeLists.txt", "add_subdirectory(Tests)\n")
        self.write("Tools/MuImport/Private/Main.cpp", "")
        self.write("Tools/MuImport/Tests/ImporterTest.cpp", "")
        self.add_to_build("Tools/MuImport")
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_tool_without_tests_is_refused(self) -> None:
        self.write("Tools/MuImport/CMakeLists.txt", "")
        self.write("Tools/MuImport/Private/Main.cpp", "")
        self.add_to_build("Tools/MuImport")
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_a_module_must_be_added_to_the_build(self) -> None:
        self.write("CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the top-level")

    def test_a_module_needs_a_private_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Private")
        self.assert_refused(structure_check.run(self.root), "needs a Private folder")

    def test_a_module_needs_public_headers_in_the_right_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Public")
        self.assert_refused(structure_check.run(self.root), "Public/Nexum/Core")

    def test_public_folders_written_with_the_wrong_case_are_refused(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Public")
        self.write(f"{CORE}/Public/nexum/core/Version.hpp", "")
        self.assert_refused(structure_check.run(self.root), "Public/Nexum/Core")

    def test_a_module_needs_tests(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Tests")
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_an_empty_tests_folder_is_not_enough(self) -> None:
        (self.root / f"{CORE}/Tests/VersionTest.cpp").unlink()
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_the_tests_of_a_module_must_be_added_to_its_build(self) -> None:
        self.write(f"{CORE}/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the build of the module")

    def test_unused_folders_cannot_appear(self) -> None:
        for folder in ("src", "include", "source", "tools"):
            with self.subTest(folder=folder):
                self.write(f"{folder}/A.cpp", "")
                self.assert_refused(structure_check.run(self.root), "this folder is not used")
                shutil.rmtree(self.root / folder)

    def test_the_tools_folder_of_the_spec_is_not_confused_with_the_lower_case_one(self) -> None:
        self.write("Tools/README.md", "")
        self.assertEqual(structure_check.run(self.root), [])

    def test_cpp_files_must_be_pascal_case(self) -> None:
        for name in ("tick_clock.cpp", "tickClock.hpp", "Tick_Clock.cpp"):
            with self.subTest(name=name):
                self.write(f"{CORE}/Private/{name}", "")
                self.assert_refused(structure_check.run(self.root), "PascalCase")
                (self.root / f"{CORE}/Private/{name}").unlink()

    def test_the_rule_also_applies_to_the_tests_of_the_repository(self) -> None:
        self.write("tests/architecture/use_after_free.cpp", "")
        self.assert_refused(structure_check.run(self.root), "PascalCase")

    def test_test_files_must_end_with_test(self) -> None:
        self.write(f"{CORE}/Tests/Version.cpp", "")
        self.assert_refused(structure_check.run(self.root), "Test.cpp")

    def test_files_of_the_original_game_are_refused_anywhere(self) -> None:
        for path in ("Content/Player.bmd", "Content/Grass.OZJ", "docs/sample.ozt", "Tools/MuImport/Tests/Light.ozb",
                     "Content/World1/EncTerrain1.att", "Content/World1/EncTerrain1.map", "Content/terrain1.att",
                     "Content/TerrainHeight.OZB"):
            with self.subTest(path=path):
                self.write(path, "")
                self.assert_refused(structure_check.run(self.root), "never enter the repository (INV-17)")
                (self.root / path).unlink()

    def test_own_content_with_ordinary_names_is_allowed(self) -> None:
        for path in ("Content/Meshes/Cube.glb", "Content/Textures/Checker.ktx2", "Content/Maps/Small.json"):
            self.write(path, "")
        self.assertEqual(structure_check.run(self.root), [])


if __name__ == "__main__":
    unittest.main()
