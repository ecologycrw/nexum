nexum_add_tests(nexum_core_tests
    SOURCES
        AssertTest.cpp
        HandlePoolTest.cpp
        ResultTest.cpp
        VersionTest.cpp
    DEPS Nexum::Core)

# A false NEXUM_ASSERT ends the development build with the condition, the file and the line, and the player build
# goes on (specs/0000-fundacao/spec.md, CA-04). The output decides; the exit code of an aborted program differs
# from one system to another.
nexum_add_tests(nexum_core_assert_failure_tests
    SOURCES AssertFailureTest.cpp
    DEPS Nexum::Core)
set(went_on "the program went on after a false assert")
if(NEXUM_AUTHORING)
    set_tests_properties(nexum_core_assert_failure_tests PROPERTIES
        PASS_REGULAR_EXPRESSION "NEXUM_ASSERT failed: cells \\+ 1 == 4 at [^\n]*AssertFailureTest\\.cpp:[0-9]+"
        FAIL_REGULAR_EXPRESSION "${went_on}")
else()
    set_tests_properties(nexum_core_assert_failure_tests PROPERTIES
        PASS_REGULAR_EXPRESSION "${went_on}"
        FAIL_REGULAR_EXPRESSION "NEXUM_ASSERT failed")
endif()
