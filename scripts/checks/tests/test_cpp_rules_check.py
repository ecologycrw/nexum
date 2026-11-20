import unittest

from support import TempRepositoryTest

from nexum_checks import cpp_rules_check

ALLOCATION_COUNTER = "tests/architecture/frame_allocations"


class CppRulesTests(TempRepositoryTest):
    def findings_for(self, code: str, path: str = "Engine/Core/Private/A.cpp") -> list:
        self.write(path, code)
        return cpp_rules_check.run(self.root)

    def test_loose_new_is_refused(self) -> None:
        self.assert_refused(self.findings_for("int* a = new int(1);\n"), "'new' is forbidden")

    def test_loose_delete_is_refused(self) -> None:
        self.assert_refused(self.findings_for("void f(int* a) { delete a; }\n"), "'delete' is forbidden")

    def test_array_delete_is_refused(self) -> None:
        self.assert_refused(self.findings_for("void f(int* a) { delete[] a; }\n"), "'delete' is forbidden")

    def test_deleted_functions_are_allowed(self) -> None:
        self.assertEqual(self.findings_for("struct A { A(const A&) = delete; A& operator=(A&&) =\n delete; };\n"), [])

    def test_manual_memory_is_refused(self) -> None:
        for call in ("malloc(4)", "std::calloc(1, 4)", "realloc(p, 4)", "free(p)"):
            with self.subTest(call=call):
                self.assert_refused(self.findings_for(f"auto a = {call};\n"), "manual memory")

    def test_functions_of_other_libraries_with_similar_names_are_allowed(self) -> None:
        self.assertEqual(self.findings_for("void f() { SDL_free(p); release_free(p); }\n"), [])

    def test_unbounded_string_functions_are_refused(self) -> None:
        for call in ("strcpy(a, b)", "sprintf(a, b)", "std::strcat(a, b)", "scanf(a)"):
            with self.subTest(call=call):
                self.assert_refused(self.findings_for(f"void f() {{ {call}; }}\n"), "C string functions")

    def test_the_system_rand_is_refused_everywhere(self) -> None:
        for path in ("Engine/Core/Private/A.cpp", "Engine/Platform/Private/A.cpp", "Games/MuOnline/Private/A.cpp",
                     "Tools/MuImport/Private/A.cpp", "tests/architecture/OtherGame/A.cpp"):
            for code in ("rand()", "std::srand(1)"):
                with self.subTest(path=path, code=code):
                    self.assert_refused(self.findings_for(f"void f() {{ {code}; }}\n", path), "INV-07")

    def test_random_device_is_allowed_only_in_the_platform_module(self) -> None:
        code = "void f() { std::random_device device; }\n"
        self.assertEqual(self.findings_for(code, "Engine/Platform/Private/A.cpp"), [])
        self.assert_refused(self.findings_for(code, "Kit/GridRpg/Private/A.cpp"), "INV-07")

    def test_system_clocks_are_refused_outside_platform_time_and_tools(self) -> None:
        for code in ("std::chrono::steady_clock::now()", "std::chrono::system_clock::now()"):
            with self.subTest(code=code):
                self.assert_refused(self.findings_for(f"void f() {{ {code}; }}\n"), "INV-05")

    def test_platform_time_and_tools_may_read_the_system_clock(self) -> None:
        code = "auto Now() { return std::chrono::steady_clock::now(); }\n"
        for path in ("Engine/Platform/Private/A.cpp", "Engine/Time/Private/A.cpp", "Tools/MuImport/Private/A.cpp"):
            with self.subTest(path=path):
                self.assertEqual(self.findings_for(code, path), [])
                (self.root / path).unlink()

    def test_the_tests_of_the_platform_module_cannot_read_the_system_clock(self) -> None:
        code = "auto Now() { return std::chrono::steady_clock::now(); }\n"
        self.assert_refused(self.findings_for(code, "Engine/Platform/Tests/ATest.cpp"), "INV-05")

    def test_reinterpreting_bytes_is_refused(self) -> None:
        for path in ("Engine/Core/Private/A.cpp", "Games/MuOnline/Private/A.cpp", "Tools/MuImport/Private/A.cpp"):
            with self.subTest(path=path):
                self.assert_refused(self.findings_for("auto p = reinterpret_cast<const H*>(bytes);\n", path), "INV-12")

    def test_throwing_is_refused_in_the_runtime(self) -> None:
        for path in ("Engine/Core/Private/A.cpp", "Kit/GridRpg/Private/A.cpp", "Games/MuOnline/Private/A.cpp",
                     "Engine/Core/Tests/ATest.cpp"):
            with self.subTest(path=path):
                self.assert_refused(self.findings_for("void f() { throw 1; }\n", path), "INV-14")

    def test_offline_tools_and_the_tests_folder_may_throw(self) -> None:
        for path in ("Tools/MuImport/Private/A.cpp", "tests/support/A.cpp"):
            with self.subTest(path=path):
                self.assertEqual(self.findings_for("void f() { throw 1; }\n", path), [])
                (self.root / path).unlink()

    def test_comments_and_strings_are_ignored(self) -> None:
        code = '// new delete malloc(1) throw\n/* strcpy(a, b) */\nauto a = "new delete free(p)";\nauto b = R"x(rand())x";\n'
        self.assertEqual(self.findings_for(code), [])

    def test_the_line_of_the_problem_is_reported(self) -> None:
        self.assert_refused(self.findings_for("// first\n\nint* a = new int(1);\n"), "line 3")

    def test_rules_also_apply_to_programs_and_tests(self) -> None:
        for path in ("Games/MuOnline/App/Private/Main.cpp", "Engine/Core/Tests/ATest.cpp",
                     "Kit/GridRpg/Private/A.cpp", "tests/architecture/ATest.cpp", "Tools/MuImport/Private/A.cpp"):
            with self.subTest(path=path):
                self.assert_refused(self.findings_for("int* a = new int(1);\n", path), "'new' is forbidden")

    def test_the_sanitizer_canaries_are_exempt(self) -> None:
        self.assertEqual(self.findings_for("int* a = new int(1);\n", "tests/architecture/sanitizer_canary/A.cpp"), [])

    def test_the_allocation_counter_may_replace_the_allocation_functions(self) -> None:
        code = (
            "void* operator new(std::size_t size) { return std::malloc(size); }\n"
            "void operator delete(void* pointer) noexcept { std::free(pointer); }\n"
        )
        self.assertEqual(self.findings_for(code, f"{ALLOCATION_COUNTER}/AllocationCounter.cpp"), [])

    def test_the_allocation_counter_follows_every_other_rule(self) -> None:
        cases = (
            ("void f() { strcpy(a, b); }\n", "C string functions"),
            ("auto a = rand();\n", "INV-07"),
            ("auto a = std::chrono::steady_clock::now();\n", "INV-05"),
            ("auto p = reinterpret_cast<const H*>(bytes);\n", "INV-12"),
        )
        for code, expected in cases:
            with self.subTest(code=code):
                self.assert_refused(self.findings_for(code, f"{ALLOCATION_COUNTER}/AllocationCounter.cpp"), expected)


if __name__ == "__main__":
    unittest.main()
