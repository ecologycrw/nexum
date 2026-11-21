nexum_add_tests(nexum_time_tests
    SOURCES
        MonotonicClockTest.cpp
        TickClockTest.cpp
        TickDiagnosticsTest.cpp
        TimersTest.cpp
    DEPS Nexum::Time)
