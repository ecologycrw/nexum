# The log and the profiler of the engine (D-18 of the reference spec).
nexum_add_library(nexum_debug
    ALIAS Nexum::Debug
    SOURCES
        Private/Logger.cpp
        Private/LogLine.cpp
        Private/Profiler.cpp
        Private/TerminalLogSink.cpp)

# The profiler macros (Nexum/Debug/Profiler.hpp) reach Tracy only when NEXUM_PROFILER is on. The definition is
# public, so every target that uses the macros sees the same value. Tracy itself stays private: only
# Private/Profiler.cpp includes it (INV-10).
target_compile_definitions(nexum_debug PUBLIC NEXUM_PROFILER_ENABLED=$<BOOL:${NEXUM_PROFILER}>)
if(NEXUM_PROFILER)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(nexum_debug PRIVATE Tracy::TracyClient)
endif()

add_subdirectory(Tests)
