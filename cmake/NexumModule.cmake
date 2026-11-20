# The only way to declare targets in this repository, so every target gets the same rules.
# This file is a protected "trava" (see docs/WORKFLOW.md).

# C++20 without compiler extensions, with every warning treated as an error (INV-15).
function(nexum_apply_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX /permissive- /utf-8 /Zc:__cplusplus /external:anglebrackets /external:W0)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror)
    endif()
endfunction()

# nexum_add_library(name [ALIAS Nexum::Name] SOURCES ... [PUBLIC_DEPS ...] [PRIVATE_DEPS ...])
# Only the Public folder is visible to the targets that link the library (docs/adr/0022-*.md).
function(nexum_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "ALIAS" "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS")
    add_library(${name} STATIC ${ARG_SOURCES})
    if(ARG_ALIAS)
        add_library(${ARG_ALIAS} ALIAS ${name})
    endif()
    target_include_directories(${name} PUBLIC Public)
    target_link_libraries(${name} PUBLIC ${ARG_PUBLIC_DEPS} PRIVATE ${ARG_PRIVATE_DEPS})
    nexum_apply_defaults(${name})
endfunction()

# nexum_add_program(name SOURCES ... [DEPS ...])
function(nexum_add_program name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPS")
    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    nexum_apply_defaults(${name})
endfunction()

# nexum_add_tests(name SOURCES ... [DEPS ...] [LABELS ...])
# Every test program shares the main function of tests/support, which writes failures as text.
function(nexum_add_tests name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPS;LABELS")
    nexum_add_program(${name} SOURCES ${ARG_SOURCES} DEPS ${ARG_DEPS} nexum_test_main doctest::doctest)
    add_test(NAME ${name} COMMAND ${name})
    if(ARG_LABELS)
        set_tests_properties(${name} PROPERTIES LABELS "${ARG_LABELS}")
    endif()
endfunction()
