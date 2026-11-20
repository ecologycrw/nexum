nexum_add_tests(nexum_debug_tests
    SOURCES
        LoggerTest.cpp
    DEPS Nexum::Debug)

# The terminal output is checked by what reaches the terminal (specs/0000-fundacao/spec.md, CA-05).
nexum_add_tests(nexum_debug_terminal_log_tests
    SOURCES TerminalLogSinkTest.cpp
    DEPS Nexum::Debug)
set_tests_properties(nexum_debug_terminal_log_tests PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[warning\\] render: the swapchain was rebuilt\n"
    FAIL_REGULAR_EXPRESSION "Status: FAILURE")
