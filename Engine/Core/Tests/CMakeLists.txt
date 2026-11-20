nexum_add_tests(nexum_core_tests
    SOURCES
        AssertTest.cpp
        HandlePoolTest.cpp
        ResultTest.cpp
        VersionTest.cpp
    DEPS Nexum::Core)

# A false NEXUM_ASSERT ends the development build with the condition, the file and the line, and the player build
# goes on (specs/0000-fundacao/spec.md, CA-04). The program does not run as a test of its own: a script runs it
# and checks how it ended, because CTest cannot accept a program ended by a signal.
nexum_add_program(nexum_core_assert_failure_program
    SOURCES AssertFailureTest.cpp
    DEPS Nexum::Core nexum_test_main doctest::doctest)
add_test(NAME nexum_core_assert_failure_tests
    COMMAND "${CMAKE_COMMAND}"
        -DPROGRAM=$<TARGET_FILE:nexum_core_assert_failure_program>
        -DASSERTS_ENABLED=${NEXUM_AUTHORING}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckAssertFailure.cmake")
