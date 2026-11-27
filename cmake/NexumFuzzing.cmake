# Fuzzing: every reader of outside data gets a fuzz target (docs/WORKFLOW.md, "Definição de pronto", and V.3 of the
# reference spec). This file is a protected "trava" (see docs/WORKFLOW.md).
#
# A fuzz target is a source file that defines LLVMFuzzerTestOneInput and a folder of seeds. Without NEXUM_FUZZING,
# on every system, it becomes a normal test that runs the function once on each seed (tests/support). With
# NEXUM_FUZZING, only with Clang, it also becomes a libFuzzer program, and the CI runs each one for a while.
option(NEXUM_FUZZING "Build the fuzz targets with libFuzzer (Clang only)" OFF)

if(NEXUM_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
        message(FATAL_ERROR "NEXUM_FUZZING needs Clang with libFuzzer: use the linux-fuzz preset.")
    endif()
    # The whole build is instrumented, so libFuzzer sees the coverage of the code under test, and every fuzz
    # target runs with the memory and undefined behavior sanitizers.
    add_compile_options(-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=all
                        -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
    # The list of fuzz targets and their seeds, one per line, for the CI step that runs each of them.
    set(NEXUM_FUZZ_LIST "${CMAKE_BINARY_DIR}/fuzz/targets.txt")
    file(WRITE "${NEXUM_FUZZ_LIST}" "")
    add_custom_target(nexum_fuzz_targets)
endif()

# nexum_add_fuzz_target(name SOURCES ... SEEDS folder [DEPS ...])
# Declares the test <name>_seeds and, with NEXUM_FUZZING, the libFuzzer program <name>_fuzz in build/<preset>/fuzz.
function(nexum_add_fuzz_target name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SEEDS" "SOURCES;DEPS")
    if(NOT ARG_SEEDS OR NOT IS_DIRECTORY "${ARG_SEEDS}")
        message(FATAL_ERROR "The fuzz target ${name} needs a folder of seeds (SEEDS), and '${ARG_SEEDS}' is not one.")
    endif()

    nexum_add_program(${name}_seeds SOURCES ${ARG_SOURCES} DEPS ${ARG_DEPS} nexum_fuzz_seed_main)
    add_test(NAME ${name}_seeds COMMAND ${name}_seeds "${ARG_SEEDS}")

    if(NEXUM_FUZZING)
        nexum_add_program(${name}_fuzz SOURCES ${ARG_SOURCES} DEPS ${ARG_DEPS})
        target_link_options(${name}_fuzz PRIVATE -fsanitize=fuzzer)
        set_target_properties(${name}_fuzz PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fuzz")
        add_dependencies(nexum_fuzz_targets ${name}_fuzz)
        file(APPEND "${NEXUM_FUZZ_LIST}" "${name}_fuzz ${ARG_SEEDS}\n")
    endif()
endfunction()
