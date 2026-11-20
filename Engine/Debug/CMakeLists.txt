# The log and, from T10 on, the profiler of the engine (D-18 of the reference spec).
nexum_add_library(nexum_debug
    ALIAS Nexum::Debug
    SOURCES
        Private/Logger.cpp
        Private/TerminalLogSink.cpp)

add_subdirectory(Tests)
