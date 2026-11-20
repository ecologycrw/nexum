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
        # Every test runs with the list of leaks inside the system's libraries that LeakSanitizer ignores.
        if(CMAKE_VERSION VERSION_LESS 3.29)
            message(FATAL_ERROR "The sanitizer build needs CMake 3.29 or newer to give the tests "
                                "the leak exceptions of cmake/LeakSuppressions.txt.")
        endif()
        set(CMAKE_TEST_LAUNCHER "${CMAKE_COMMAND}" -E env
            "LSAN_OPTIONS=suppressions='${CMAKE_CURRENT_LIST_DIR}/LeakSuppressions.txt'")
    endif()
endif()
