nexum_add_tests(nexum_debug_tests
    SOURCES
        LoggerTest.cpp
        ProfilerTest.cpp
    DEPS Nexum::Debug nexum_test_log)

# The terminal output is checked by what reaches the terminal (specs/0000-fundacao/spec.md, CA-05).
nexum_add_tests(nexum_debug_terminal_log_tests
    SOURCES TerminalLogSinkTest.cpp
    DEPS Nexum::Debug)
set_tests_properties(nexum_debug_terminal_log_tests PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[warning\\] render: the swapchain was rebuilt\n"
    FAIL_REGULAR_EXPRESSION "Status: FAILURE")

# The player build refuses the profiler, and the development build accepts it (CA-05). Each test runs the check
# of the root CMakeLists.txt with one combination of the options. When a test has a pass expression, CTest looks
# only at the output, so the refused case passes on the error message although the script fails. CMake breaks
# long error messages into lines, so the refused case looks only for the start of the message.
function(nexum_add_profiler_option_test name profiler authoring expected)
    add_test(NAME ${name}
        COMMAND "${CMAKE_COMMAND}" -DPROFILER=${profiler} -DAUTHORING=${authoring}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckProfilerOption.cmake")
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${expected}")
endfunction()

nexum_add_profiler_option_test(nexum_debug_profiler_refused_in_player_build ON OFF
    "NEXUM_PROFILER=ON needs NEXUM_AUTHORING=ON")
set_tests_properties(nexum_debug_profiler_refused_in_player_build PROPERTIES
    FAIL_REGULAR_EXPRESSION "was accepted")
nexum_add_profiler_option_test(nexum_debug_profiler_accepted_in_development_build ON ON
    "The profiler option was accepted: NEXUM_PROFILER=ON, NEXUM_AUTHORING=ON")
nexum_add_profiler_option_test(nexum_debug_profiler_off_in_player_build OFF OFF
    "The profiler option was accepted: NEXUM_PROFILER=OFF, NEXUM_AUTHORING=OFF")
