nexum_add_tests(nexum_platform_tests
    SOURCES
        ApplicationTest.cpp
        FailureReportsTest.cpp
        PlatformInfoTest.cpp
        PngTest.cpp
        SdlLogRouteTest.cpp
        VideoSystemTest.cpp
        WindowTest.cpp
    DEPS Nexum::Platform nexum_test_log)

# A fuzz target for each reader of outside data: its seeds run here as a test on every system.
nexum_add_fuzz_target(nexum_platform_load_png
    SOURCES LoadPngFuzzTest.cpp
    SEEDS "${CMAKE_CURRENT_SOURCE_DIR}/FuzzSeeds/LoadPng"
    DEPS Nexum::Platform)

# CA-12 of slice 1: ReadHiddenLine reads one line of the standard input, without its end and within its limit. The
# program reads from files, as from a pipe, so a script runs it with each input. CMake puts its test launcher only
# in front of the programs of the build, never in front of a script, so the script gets it here, with what the
# launcher sets, such as the leak exceptions of the sanitizer build (cmake/NexumSanitizers.cmake).
nexum_add_program(nexum_platform_hidden_line_program
    SOURCES HiddenLineTest.cpp
    DEPS Nexum::Platform)
add_test(NAME nexum_platform_hidden_line_tests
    COMMAND ${CMAKE_TEST_LAUNCHER} "${CMAKE_COMMAND}"
        -DPROGRAM=$<TARGET_FILE:nexum_platform_hidden_line_program>
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/hidden_line_test"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckHiddenLine.cmake")
