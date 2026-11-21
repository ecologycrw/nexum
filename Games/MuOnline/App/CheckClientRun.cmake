# Runs MuClient and checks both how it ended and what it wrote (specs/0000-fundacao/spec.md, CA-06 and CA-10). A
# plain CTest test checks the exit code or the output, never both.
#
# cmake -DPROGRAM=<path> -DARGUMENTS=<list> -DEXIT_CODE=<number> -DPATTERNS=<list of regular expressions>
#       -P CheckClientRun.cmake

execute_process(COMMAND "${PROGRAM}" ${ARGUMENTS}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output TIMEOUT 300)

string(REPLACE ";" " " command_line "${ARGUMENTS}")
# The result is the exit code, or a text such as "Child aborted" when a signal ended the program.
if(NOT result STREQUAL "${EXIT_CODE}")
    message(FATAL_ERROR "MuClient ${command_line} ended with ${result} instead of ${EXIT_CODE}:\n${output}")
endif()
foreach(pattern IN LISTS PATTERNS)
    if(NOT output MATCHES "${pattern}")
        message(FATAL_ERROR "the output of MuClient ${command_line} misses \"${pattern}\":\n${output}")
    endif()
endforeach()

message(STATUS "MuClient ${command_line} ended with ${result}, as expected:\n${output}")
