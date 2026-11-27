# Runs the packet generator (Tools/PacketGen) during the build, so the code of the packets is never stored in the
# repository (docs/adr/0034-packetgen-em-python-no-build.md). This file is a protected "trava" (see docs/WORKFLOW.md).
#
# The generator is a Python script with the standard library only, which the checks already need. It also runs in
# the player build, because the game reads its packets with the generated code; the script itself never enters the
# program.

find_package(Python3 3.10 COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR
        "The packet generator (Tools/PacketGen) needs Python 3.10 or newer, and CMake did not find it. "
        "Install Python 3 (docs/setup/DEV.md) and configure again.")
endif()

set(NEXUM_PACKETGEN_SCRIPT "${CMAKE_SOURCE_DIR}/Tools/PacketGen/Private/packetgen.py")

# nexum_generate_packets(XML file NAMESPACE ns OUTPUT_DIR dir INCLUDE_DIR Mu/.../Name HEADERS_VARIABLE var
#                        [TESTS_PREFIX Tests/NameTest TESTS_VARIABLE var]
#                        [COMMON file COMMON_NAMESPACE ns COMMON_INCLUDE include] [PEERS file...])
# Writes, below OUTPUT_DIR/INCLUDE_DIR, Types.hpp and one header for each packet, named after it; the code includes
# a packet as INCLUDE_DIR/<Packet>.hpp, with OUTPUT_DIR on the include path, so it compiles only the packets it uses
# (proposal P-0016). With TESTS_PREFIX, the tests go to OUTPUT_DIR/TESTS_PREFIX01.cpp, 02.cpp... The variables
# receive the paths of the generated files, which go into the sources of a target so the build generates them
# before compiling it.
#
# The build must know every file before the generator runs, so the generator lists them when CMake configures. A
# change in the XML files or in the script configures again, because it may add or remove packets.
function(nexum_generate_packets)
    set(one_value XML NAMESPACE OUTPUT_DIR INCLUDE_DIR TESTS_PREFIX COMMON COMMON_NAMESPACE COMMON_INCLUDE
        HEADERS_VARIABLE TESTS_VARIABLE)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "${one_value}" "PEERS")
    set(command "${Python3_EXECUTABLE}" -I "${NEXUM_PACKETGEN_SCRIPT}"
        --xml "${ARG_XML}" --namespace "${ARG_NAMESPACE}" --output-dir "${ARG_OUTPUT_DIR}"
        --include-dir "${ARG_INCLUDE_DIR}")
    set(inputs "${ARG_XML}" "${NEXUM_PACKETGEN_SCRIPT}")
    if(ARG_TESTS_PREFIX)
        list(APPEND command --tests-prefix "${ARG_TESTS_PREFIX}")
    endif()
    if(ARG_COMMON)
        list(APPEND command --common "${ARG_COMMON}" --common-namespace "${ARG_COMMON_NAMESPACE}"
            --common-include "${ARG_COMMON_INCLUDE}")
        list(APPEND inputs "${ARG_COMMON}")
    endif()
    foreach(peer IN LISTS ARG_PEERS)
        list(APPEND command --peer "${peer}")
        list(APPEND inputs "${peer}")
    endforeach()
    get_filename_component(xml_name "${ARG_XML}" NAME)

    execute_process(
        COMMAND ${command} --list
        OUTPUT_VARIABLE listed
        ERROR_VARIABLE list_error
        RESULT_VARIABLE list_result)
    if(NOT list_result EQUAL 0)
        message(FATAL_ERROR "The packet generator could not list the files of ${xml_name}: ${list_error}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${inputs})
    string(REPLACE "\n" ";" listed "${listed}")
    set(headers "")
    set(tests "")
    foreach(line IN LISTS listed)
        if(line MATCHES "^header (.+)$")
            list(APPEND headers "${CMAKE_MATCH_1}")
        elseif(line MATCHES "^tests (.+)$")
            list(APPEND tests "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${headers} ${tests}
        COMMAND ${command}
        DEPENDS ${inputs}
        COMMENT "Generating the packets of ${xml_name}"
        VERBATIM)
    set(${ARG_HEADERS_VARIABLE} "${headers}" PARENT_SCOPE)
    if(ARG_TESTS_VARIABLE)
        set(${ARG_TESTS_VARIABLE} "${tests}" PARENT_SCOPE)
    endif()
endfunction()
