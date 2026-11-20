import unittest

from support import TempRepositoryTest

from nexum_checks import architecture_check

CORE = "Engine/Core"
PLATFORM = "Engine/Platform"
RENDER = "Engine/Render"
KIT = "Kit/GridRpg"
GAME = "Games/MuOnline"
APP = "Games/MuOnline/App"
OTHER_GAME = "tests/architecture/OtherGame"


class LayerTests(TempRepositoryTest):
    def refused(self, path: str, content: str, expected: str) -> None:
        self.write(path, content)
        self.assert_refused(architecture_check.run(self.root), expected)

    def test_the_engine_cannot_include_the_kit(self) -> None:
        for folder in (CORE, PLATFORM, RENDER):
            with self.subTest(folder=folder):
                self.refused(f"{folder}/Private/A.cpp", '#include "Nexum/GridRpg/Grid/TileGrid.hpp"\n', "INV-02")

    def test_the_engine_cannot_include_the_game_module(self) -> None:
        for folder in (CORE, PLATFORM, RENDER):
            with self.subTest(folder=folder):
                self.refused(f"{folder}/Private/A.cpp", '#include "Mu/Protocol/Framing.hpp"\n', "INV-02")

    def test_the_kit_cannot_include_the_game_module(self) -> None:
        self.refused(f"{KIT}/Private/A.cpp", '#include "Mu/ModuleIdentity.hpp"\n', "INV-02")

    def test_the_other_game_cannot_include_the_game_module(self) -> None:
        self.refused(f"{OTHER_GAME}/Private/A.cpp", '#include "Mu/ModuleIdentity.hpp"\n', "INV-02")

    def test_the_core_cannot_include_other_engine_modules(self) -> None:
        for header in ("Nexum/Platform/PlatformInfo.hpp", "Nexum/Render/Passes.hpp", "Nexum/Net/Transport.hpp"):
            with self.subTest(header=header):
                self.refused(f"{CORE}/Private/A.cpp", f'#include "{header}"\n', "Engine/Core is the base")

    def test_other_engine_modules_may_include_each_other(self) -> None:
        self.write(f"{RENDER}/Private/A.cpp", '#include "Nexum/Platform/Window.hpp"\n#include "Nexum/Core/Result.hpp"\n')
        self.assertEqual(architecture_check.run(self.root), [])

    def test_an_include_with_the_wrong_case_does_not_escape_the_rules(self) -> None:
        for header in ("nexum/gridrpg/x.hpp", "NEXUM/GRIDRPG/X.hpp", "mu/x.hpp", "MU/X.hpp"):
            with self.subTest(header=header):
                self.refused(f"{CORE}/Private/A.cpp", f'#include "{header}"\n', "INV-02")

    def test_the_public_headers_and_the_tests_follow_the_same_rules(self) -> None:
        for path in (f"{CORE}/Public/Nexum/Core/A.hpp", f"{CORE}/Tests/ATest.cpp", f"{KIT}/Tests/ATest.cpp"):
            with self.subTest(path=path):
                self.refused(path, '#include "Mu/X.hpp"\n', "INV-02")

    def test_the_game_module_may_use_the_kit_and_the_engine(self) -> None:
        self.write(f"{GAME}/Private/A.cpp", '#include "Nexum/GridRpg/A.hpp"\n#include "Nexum/Platform/B.hpp"\n')
        self.write(f"{APP}/Private/Main.cpp", '#include "Mu/ModuleIdentity.hpp"\n#include "Nexum/Core/Result.hpp"\n')
        self.assertEqual(architecture_check.run(self.root), [])

    def test_the_engine_cannot_link_the_kit_or_the_game_module(self) -> None:
        for target in ("nexum_gridrpg", "Nexum::GridRpg", "MuOnline", "MuClient", "muonline_tests"):
            with self.subTest(target=target):
                self.refused(f"{PLATFORM}/CMakeLists.txt", f"nexum_add_library(a PUBLIC_DEPS {target})\n", "INV-02")

    def test_the_kit_cannot_link_the_game_module(self) -> None:
        self.refused(f"{KIT}/CMakeLists.txt", "nexum_add_library(a PUBLIC_DEPS MuOnline)\n", "INV-02")

    def test_the_kit_and_the_other_game_may_link_the_kit(self) -> None:
        self.write(f"{KIT}/CMakeLists.txt", "nexum_add_library(nexum_gridrpg ALIAS Nexum::GridRpg)\n")
        self.write(f"{OTHER_GAME}/CMakeLists.txt", "nexum_add_library(other_game PUBLIC_DEPS Nexum::GridRpg)\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_names_that_only_resemble_the_game_module_are_accepted(self) -> None:
        self.write(f"{KIT}/CMakeLists.txt", "nexum_add_library(menu_online_x PRIVATE_DEPS my_muclient2)\n")
        self.write(f"{KIT}/Private/A.cpp", '#include "Nexum/GridRpg/Menu/X.hpp"\n')
        self.assertEqual(architecture_check.run(self.root), [])

    def test_a_commented_include_is_ignored(self) -> None:
        self.write(f"{CORE}/Private/A.cpp", '// #include <SDL3/SDL.h>\n/* #include "Mu/X.hpp" */\n')
        self.assertEqual(architecture_check.run(self.root), [])


class OwnerTests(TempRepositoryTest):
    def refused(self, path: str, content: str, expected: str = "INV-10") -> None:
        self.write(path, content)
        self.assert_refused(architecture_check.run(self.root), expected)

    def test_sdl_is_refused_outside_its_modules(self) -> None:
        for path in (f"{CORE}/Private/A.cpp", f"{KIT}/Private/A.cpp", f"{GAME}/Private/A.cpp",
                     f"{APP}/Private/Main.cpp", f"{OTHER_GAME}/A.cpp", "tests/support/A.cpp"):
            with self.subTest(path=path):
                self.refused(path, "#include <SDL3/SDL.h>\n")

    def test_sdl_is_refused_in_the_public_headers_and_tests_of_the_platform_module(self) -> None:
        for path in (f"{PLATFORM}/Public/Nexum/Platform/A.hpp", f"{PLATFORM}/Tests/ATest.cpp"):
            with self.subTest(path=path):
                self.refused(path, "#include <SDL3/SDL.h>\n")

    def test_the_platform_and_render_modules_may_use_sdl(self) -> None:
        self.write(f"{PLATFORM}/Private/A.cpp", "#include <SDL3/SDL.h>\n#include <windows.h>\n")
        self.write(f"{PLATFORM}/CMakeLists.txt", "nexum_add_library(a PRIVATE_DEPS SDL3::SDL3)\n")
        self.write(f"{RENDER}/Private/A.cpp", "#include <SDL3/SDL_gpu.h>\n")
        self.write(f"{RENDER}/CMakeLists.txt", "nexum_add_library(b PRIVATE_DEPS SDL3::SDL3)\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_the_tests_of_the_platform_module_cannot_link_sdl(self) -> None:
        self.refused(f"{PLATFORM}/Tests/CMakeLists.txt", "nexum_add_tests(a DEPS SDL3::SDL3)\n")

    def test_a_program_cannot_link_sdl_directly(self) -> None:
        self.refused(f"{APP}/CMakeLists.txt", "nexum_add_program(a DEPS SDL3::SDL3)\n")

    def test_operating_system_headers_are_refused_outside_the_platform_module(self) -> None:
        for header in ("windows.h", "Windows.h", "unistd.h", "sys/socket.h", "winsock2.h"):
            with self.subTest(header=header):
                self.refused(f"{RENDER}/Private/A.cpp", f"#include <{header}>\n")

    def test_each_engine_library_stays_in_its_module(self) -> None:
        cases = (
            ("#include <asio.hpp>\n", "Engine/Net/Private/A.cpp"),
            ("#include <entt/entity/registry.hpp>\n", "Engine/World/Private/A.cpp"),
            ("#include <RmlUi/Core.h>\n", "Engine/Ui/Private/A.cpp"),
            ("#include <imgui.h>\n", "Engine/Debug/Private/A.cpp"),
            ("#include <tracy/Tracy.hpp>\n", "Engine/Debug/Private/A.cpp"),
            ("#include <miniaudio.h>\n", "Engine/Audio/Private/A.cpp"),
            ("#include <glm/vec3.hpp>\n", "Engine/Core/Private/A.cpp"),
        )
        for content, owner in cases:
            with self.subTest(content=content):
                self.write(owner, content)
                self.assertEqual(architecture_check.run(self.root), [])
                (self.root / owner).unlink()
                self.refused(f"{GAME}/Private/A.cpp", content)
                (self.root / f"{GAME}/Private/A.cpp").unlink()
                self.refused(f"{RENDER}/Public/Nexum/Render/A.hpp", content)
                (self.root / f"{RENDER}/Public/Nexum/Render/A.hpp").unlink()

    def test_engine_libraries_cannot_be_linked_outside_their_module(self) -> None:
        for target in ("EnTT::EnTT", "imgui::imgui", "Tracy::TracyClient", "RmlUi::Core", "glm::glm", "asio::asio"):
            with self.subTest(target=target):
                self.refused(f"{GAME}/CMakeLists.txt", f"nexum_add_library(a PRIVATE_DEPS {target})\n")

    def test_the_tools_may_use_anything(self) -> None:
        self.write("Tools/MuImport/Private/Main.cpp", '#include <SDL3/SDL.h>\n#include "Mu/X.hpp"\n#include <windows.h>\n')
        self.write("Tools/MuImport/CMakeLists.txt", "nexum_add_program(muimport DEPS MuOnline SDL3::SDL3)\n")
        self.assertEqual(architecture_check.run(self.root), [])


if __name__ == "__main__":
    unittest.main()
