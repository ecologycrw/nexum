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
    endif()
endif()
