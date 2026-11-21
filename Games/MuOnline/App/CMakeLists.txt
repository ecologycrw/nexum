# The entry point of the game. Today it shows the foundation screen: the test card and the square that walks by
# tick (specs/0000-fundacao/spec.md).
nexum_add_program(MuClient
    SOURCES
        Private/FoundationApp.cpp
        Private/Main.cpp
    DEPS MuOnline Nexum::Platform)

# The player build never carries the authoring mode (INV-19). NEXUM_AUTHORING comes from the preset.
target_compile_definitions(MuClient PRIVATE NEXUM_AUTHORING_ENABLED=$<BOOL:${NEXUM_AUTHORING}>)

string(REPLACE "." "\\." version_pattern "${PROJECT_VERSION}")
add_test(NAME muclient_reports_its_version COMMAND MuClient --version)
set_tests_properties(muclient_reports_its_version PROPERTIES
    PASS_REGULAR_EXPRESSION "^MuClient ${version_pattern} \\(Nexum ${version_pattern}, Nexum GridRpg ${version_pattern}\\)")

if(NEXUM_AUTHORING)
    set(expected_build_kind "development build")
else()
    set(expected_build_kind "player build")
endif()
add_test(NAME muclient_reports_the_build_kind COMMAND MuClient --version)
set_tests_properties(muclient_reports_the_build_kind PROPERTIES PASS_REGULAR_EXPRESSION "${expected_build_kind}")

add_test(NAME muclient_refuses_unknown_arguments COMMAND MuClient --unknown)
set_tests_properties(muclient_refuses_unknown_arguments PROPERTIES WILL_FAIL TRUE)

# Runs MuClient with the arguments, and checks the exit code and every pattern of the output. The lists are
# joined with $<SEMICOLON>, so each one reaches the script as a single argument.
#
# CMake puts its test launcher only in front of the programs of the build, never in front of a script. The
# script gets it here, so MuClient runs with what the launcher sets, such as the leak exceptions of the
# sanitizer build (cmake/NexumSanitizers.cmake).
function(muclient_add_run_test name arguments exit_code patterns)
    list(JOIN arguments "$<SEMICOLON>" joined_arguments)
    list(JOIN patterns "$<SEMICOLON>" joined_patterns)
    add_test(NAME ${name}
        COMMAND ${CMAKE_TEST_LAUNCHER} "${CMAKE_COMMAND}"
            -DPROGRAM=$<TARGET_FILE:MuClient>
            "-DARGUMENTS=${joined_arguments}"
            -DEXIT_CODE=${exit_code}
            "-DPATTERNS=${joined_patterns}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckClientRun.cmake")
endfunction()

# CA-10 and CA-06: the window opens, shows 300 frames and closes by itself with success, and the log names the
# GPU backend and the graphics card. On Linux, the CI runs it inside the virtual X11 server.
muclient_add_run_test(muclient_draws_300_frames_and_closes "--frames;300" 0
    "render: GPU backend (vulkan|direct3d12|metal) on .;client: drew 300 frames")

# The command line is read before anything starts (INV-12): a value it cannot read ends the program with the
# reason and the usage.
muclient_add_run_test(muclient_refuses_an_unknown_gpu_driver "--gpu-driver;glide" 2
    "unknown GPU driver 'glide'.;Usage: MuClient")
muclient_add_run_test(muclient_refuses_zero_frames "--frames;0" 2
    "--frames needs a whole number from 1 to 4294967295, not '0';Usage: MuClient")
