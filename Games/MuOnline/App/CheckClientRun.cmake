# Runs MuClient and checks both how it ended and what it wrote (specs/0000-fundacao/spec.md, CA-06 and CA-10). A
# plain CTest test checks the exit code or the output, never both. With INPUT, the program reads that file as its
# standard input, as the password typed in the terminal; ABSENT lists texts the output must not have
# (specs/0001-rede-ate-o-mundo/spec.md, CA-12). A script that includes this one finds what the program wrote in
# `output` afterwards (tests/integration/openmu/RecordAndReplay.cmake).
#
# cmake -DPROGRAM=<path> -DARGUMENTS=<list> -DEXIT_CODE=<number> -DPATTERNS=<list of regular expressions>
#       [-DINPUT=<file>] [-DABSENT=<list of texts>] -P CheckClientRun.cmake

set(input_option "")
if(INPUT)
    set(input_option INPUT_FILE "${INPUT}")
endif()
execute_process(COMMAND "${PROGRAM}" ${ARGUMENTS} ${input_option}
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
foreach(text IN LISTS ABSENT)
    string(FIND "${output}" "${text}" found_at)
    if(NOT found_at EQUAL -1)
        message(FATAL_ERROR "the output of MuClient ${command_line} has \"${text}\", which it must not:\n${output}")
    endif()
endforeach()

message(STATUS "MuClient ${command_line} ended with ${result}, as expected:\n${output}")
