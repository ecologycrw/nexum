# The entry point of the game. Today it shows the foundation screen: the test card and the square that walks by
# tick (specs/0000-fundacao/spec.md).
nexum_add_program(MuClient
    SOURCES
        Private/ClientStart.cpp
        Private/FoundationApp.cpp
        Private/Main.cpp
    DEPS MuOnline Nexum::Platform)

# The player build never carries the authoring mode (INV-19). NEXUM_AUTHORING comes from the preset. The capture
# of the proof scene (--capture, CA-07) is compiled only into the development build.
target_compile_definitions(MuClient PRIVATE NEXUM_AUTHORING_ENABLED=$<BOOL:${NEXUM_AUTHORING}>)
if(NEXUM_AUTHORING)
    target_sources(MuClient PRIVATE Private/ProofCapture.cpp)
endif()

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
# GPU backend and the graphics card. On Linux, the tests run inside the virtual X11 server (docs/setup/DEV.md),
# and the window must open on X11.
set(frames_patterns "render: GPU backend (vulkan|direct3d12|metal) on .;client: drew 300 frames")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND frames_patterns "platform: video driver x11")
endif()
muclient_add_run_test(muclient_draws_300_frames_and_closes "--frames;300" 0 "${frames_patterns}")

# CA-06: on Windows, when Vulkan is missing, the default order falls to Direct3D 12. The Vulkan loader finds no
# driver when its list of drivers names only a file that does not exist, so Vulkan cannot start in this test.
if(WIN32)
    muclient_add_run_test(muclient_falls_back_to_direct3d12_without_vulkan "--frames;30" 0
        "render: GPU backend direct3d12 on .;client: drew 30 frames")
    set(no_vulkan_driver "${CMAKE_CURRENT_BINARY_DIR}/no-vulkan-driver/none.json")
    set_tests_properties(muclient_falls_back_to_direct3d12_without_vulkan PROPERTIES
        ENVIRONMENT "VK_DRIVER_FILES=${no_vulkan_driver};VK_ICD_FILENAMES=${no_vulkan_driver}")
endif()

# The command line is read before anything starts (INV-12): a value it cannot read ends the program with the
# reason and the usage.
muclient_add_run_test(muclient_refuses_an_unknown_gpu_driver "--gpu-driver;glide" 2
    "unknown GPU driver 'glide'.;Usage: MuClient")
muclient_add_run_test(muclient_refuses_zero_frames "--frames;0" 2
    "--frames needs a whole number from 1 to 4294967295, not '0';Usage: MuClient")

# CA-07: the development build writes the proof scene without a window, on every CI system; the comparison with
# the approved image is in tests/visual. The player build refuses --capture (INV-19).
if(NEXUM_AUTHORING)
    muclient_add_run_test(muclient_captures_the_proof_scene
        "--capture;${CMAKE_CURRENT_BINARY_DIR}/capture_test/proof-scene.png" 0
        "render: GPU backend (vulkan|direct3d12|metal) on .;client: wrote the proof scene to .*proof-scene.png")
    muclient_add_run_test(muclient_reports_a_capture_it_cannot_write
        "--capture;${CMAKE_CURRENT_BINARY_DIR}/capture_test/missing/proof-scene.png" 1
        "client: could not capture the proof scene: .*PNG file")
    # The folder of the capture exists before the tests run; its "missing" subfolder never does.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/capture_test")
else()
    muclient_add_run_test(muclient_player_build_refuses_capture "--capture;proof-scene.png" 2
        "--capture exists only in the development build;Usage: MuClient")
endif()
