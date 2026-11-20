# Runs the program with a false NEXUM_ASSERT and checks how it ended (specs/0000-fundacao/spec.md, CA-04).
# A plain CTest test cannot check it: on Linux and macOS, std::abort ends the program with a signal, and CTest marks
# such a program as failed whatever its output says.
#
# cmake -DPROGRAM=<path> -DASSERTS_ENABLED=<ON|OFF> -P CheckAssertFailure.cmake

execute_process(COMMAND "${PROGRAM}" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)

set(message_pattern "NEXUM_ASSERT failed: cells \\+ 1 == 4 at [^\n]*AssertFailureTest\\.cpp:[0-9]+")
set(went_on "the program went on after a false assert")

if(ASSERTS_ENABLED)
    # The result is the exit code, or a text such as "Child aborted" when a signal ended the program.
    if(result EQUAL 0)
        message(FATAL_ERROR "a false NEXUM_ASSERT did not end the development build:\n${output}")
    endif()
    if(NOT output MATCHES "${message_pattern}")
        message(FATAL_ERROR "the message of the false NEXUM_ASSERT is missing or wrong:\n${output}")
    endif()
    if(output MATCHES "${went_on}")
        message(FATAL_ERROR "the program went on after a false NEXUM_ASSERT:\n${output}")
    endif()
else()
    if(NOT result EQUAL 0 OR NOT output MATCHES "${went_on}" OR output MATCHES "NEXUM_ASSERT failed")
        message(FATAL_ERROR "the NEXUM_ASSERT of the player build is not off (${result}):\n${output}")
    endif()
endif()

message(STATUS "NEXUM_ASSERT ended the program as expected (${result})")
