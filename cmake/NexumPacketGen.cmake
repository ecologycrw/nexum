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

# nexum_generate_packets(XML file NAMESPACE ns OUTPUT_DIR dir INCLUDE Mu/.../Name.hpp HEADER_VARIABLE var
#                        [TESTS NameTest.cpp TESTS_VARIABLE var]
#                        [COMMON file COMMON_NAMESPACE ns COMMON_INCLUDE include] [PEERS file...])
# Writes OUTPUT_DIR/INCLUDE (the header) and, with TESTS, OUTPUT_DIR/TESTS (the tests). The code includes the header
# as INCLUDE, with OUTPUT_DIR on the include path. The variables receive the paths of the generated files, which go
# into the sources of a target so the build generates them before compiling it.
function(nexum_generate_packets)
    cmake_parse_arguments(PARSE_ARGV 0 ARG ""
        "XML;NAMESPACE;OUTPUT_DIR;TESTS;INCLUDE;COMMON;COMMON_NAMESPACE;COMMON_INCLUDE;HEADER_VARIABLE;TESTS_VARIABLE"
        "PEERS")
    set(header "${ARG_OUTPUT_DIR}/${ARG_INCLUDE}")
    set(command "${Python3_EXECUTABLE}" -I "${NEXUM_PACKETGEN_SCRIPT}"
        --xml "${ARG_XML}" --namespace "${ARG_NAMESPACE}" --header "${header}" --include "${ARG_INCLUDE}")
    set(outputs "${header}")
    set(inputs "${ARG_XML}" "${NEXUM_PACKETGEN_SCRIPT}")
    if(ARG_TESTS)
        set(tests "${ARG_OUTPUT_DIR}/${ARG_TESTS}")
        list(APPEND command --tests "${tests}")
        list(APPEND outputs "${tests}")
        set(${ARG_TESTS_VARIABLE} "${tests}" PARENT_SCOPE)
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
    add_custom_command(
        OUTPUT ${outputs}
        COMMAND ${command}
        DEPENDS ${inputs}
        COMMENT "Generating the packets of ${xml_name}"
        VERBATIM)
    set(${ARG_HEADER_VARIABLE} "${header}" PARENT_SCOPE)
endfunction()
