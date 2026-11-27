# Runs the program of ReadHiddenLine with each input and checks what it read (specs/0001-rede-ate-o-mundo/spec.md,
# CA-12): the prompt goes to the error output, the end of the line is not part of the text, only the first line is
# read, and an empty input or a line that is too long give an error. The inputs come from files, as from a pipe;
# the hiding of what is typed in a real terminal is checked by hand (the test of the owner in the spec).
#
# cmake -DPROGRAM=<path> -DWORK_DIR=<folder> -P CheckHiddenLine.cmake

file(MAKE_DIRECTORY "${WORK_DIR}")

# Writes the input with each "\n" as the end of line of `style` (LF or CRLF), the same on every system; file(WRITE)
# would write "\n" as "\r\n" on Windows. NONE writes an input without any end of line as it is.
function(write_input file style input)
    if(style STREQUAL "NONE")
        file(WRITE "${file}" "${input}")
    else()
        file(CONFIGURE OUTPUT "${file}" CONTENT "${input}" NEWLINE_STYLE ${style})
    endif()
endfunction()

function(check_line name style input exit_code expected)
    set(input_file "${WORK_DIR}/${name}.txt")
    write_input("${input_file}" ${style} "${input}")
    execute_process(COMMAND "${PROGRAM}"
        INPUT_FILE "${input_file}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE prompt TIMEOUT 30)
    if(NOT result STREQUAL "${exit_code}")
        message(FATAL_ERROR "${name}: the program ended with ${result} instead of ${exit_code}:\n${output}")
    endif()
    if(NOT output STREQUAL "${expected}\n")
        message(FATAL_ERROR "${name}: the program wrote\n${output}\ninstead of\n${expected}")
    endif()
    string(FIND "${prompt}" "Secret: " prompt_at)
    if(NOT prompt_at EQUAL 0)
        message(FATAL_ERROR "${name}: the error output does not start with the prompt:\n${prompt}")
    endif()
    message(STATUS "${name}: ${output}")
endfunction()

set(test0 "read 5 bytes: 74 65 73 74 30")
check_line(line_feed LF "test0\n" 0 "${test0}")
check_line(carriage_return_and_line_feed CRLF "test0\n" 0 "${test0}")
check_line(no_end_of_line NONE "test0" 0 "${test0}")
check_line(only_the_first_line LF "test0\nsecond\n" 0 "${test0}")
check_line(empty_line LF "\n" 0 "read 0 bytes:")
check_line(utf8_as_it_comes LF "ação\n" 0 "read 6 bytes: 61 C3 A7 C3 A3 6F")
check_line(empty_input NONE "" 1 "error: the input ended before a line was typed")

string(REPEAT "a" 256 longest)
string(REPEAT " 61" 256 longest_bytes)
check_line(longest_line CRLF "${longest}\n" 0 "read 256 bytes:${longest_bytes}")
check_line(line_too_long LF "${longest}a\n" 1 "error: the line is longer than 256 bytes")
