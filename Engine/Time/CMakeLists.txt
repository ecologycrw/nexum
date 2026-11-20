# The fixed-step clock of the simulation (docs/adr/0005-tick-fixo-de-25-hz.md).
nexum_add_library(nexum_time
    ALIAS Nexum::Time
    SOURCES
        Private/TickClock.cpp)

add_subdirectory(Tests)
