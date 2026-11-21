# The fixed-step clock of the simulation, the monotonic clock of the wall, the timers and the numbers of the tick
# loop (docs/adr/0005-tick-fixo-de-25-hz.md).
nexum_add_library(nexum_time
    ALIAS Nexum::Time
    SOURCES
        Private/MonotonicClock.cpp
        Private/TickClock.cpp
        Private/TickDiagnostics.cpp
        Private/TickTimer.cpp
        Private/WallTimer.cpp)

add_subdirectory(Tests)
