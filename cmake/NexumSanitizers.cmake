# Memory and undefined behavior sanitizers (docs/adr/0025-ferramentas-de-verificacao.md).
# This file is a protected "trava" (see docs/WORKFLOW.md).
option(NEXUM_SANITIZE "Build with the memory and undefined behavior sanitizers" OFF)

if(NEXUM_SANITIZE)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        # The third-party libraries are not built with sanitizers, so the container annotations must stay off.
        add_compile_definitions(_DISABLE_STL_ANNOTATION)
        add_link_options(/INCREMENTAL:NO)
        # AddressSanitizer cannot be combined with the runtime checks that CMake enables by default in Debug.
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
        if(CMAKE_VERSION VERSION_LESS 3.29)
            message(FATAL_ERROR "The sanitizer build needs CMake 3.29 or newer to give the tests "
                                "the environment of the leak check.")
        endif()
        # Today only Linux with Clang reaches this branch: macOS has no sanitizer build. A macOS sanitizer build
        # must keep the lavapipe requirement below to Linux, because macOS has no lavapipe and no LD_PRELOAD.
        # The GPU tests run on the Vulkan driver without a graphics card (Mesa lavapipe), as on the CI. Each test
        # loads the driver from its start, so the driver is still loaded when LeakSanitizer looks for leaks at the
        # end: unloaded before that, its own memory looked like leaks with no names (specs/0000-fundacao/tasks.md,
        # T13). Without the driver, the build stops here with the reason, instead of failing later with leaks
        # nobody can read.
        find_library(NEXUM_LAVAPIPE_DRIVER NAMES vulkan_lvp)
        if(NOT NEXUM_LAVAPIPE_DRIVER OR NOT EXISTS "${NEXUM_LAVAPIPE_DRIVER}")
            message(FATAL_ERROR "The Linux sanitizer build needs the Vulkan driver without a graphics card (Mesa "
                                "lavapipe, libvulkan_lvp.so, in the mesa-vulkan-drivers package), and it was not "
                                "found. Without it, the leak check would report the memory of the unloaded driver "
                                "as leaks. Install it as docs/setup/DEV.md says.")
        endif()
        # Every test also runs with the list of leaks inside the system's libraries that LeakSanitizer ignores.
        set(CMAKE_TEST_LAUNCHER "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=${NEXUM_LAVAPIPE_DRIVER}"
            "LSAN_OPTIONS=suppressions='${CMAKE_CURRENT_LIST_DIR}/LeakSuppressions.txt'")
    endif()
endif()
