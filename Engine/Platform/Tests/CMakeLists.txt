nexum_add_tests(nexum_platform_tests
    SOURCES
        ApplicationTest.cpp
        FailureReportsTest.cpp
        PlatformInfoTest.cpp
        PngTest.cpp
        SdlLogRouteTest.cpp
        VideoSystemTest.cpp
        WindowTest.cpp
    DEPS Nexum::Platform nexum_test_log)

# A fuzz target for each reader of outside data: its seeds run here as a test on every system.
nexum_add_fuzz_target(nexum_platform_load_png
    SOURCES LoadPngFuzzTest.cpp
    SEEDS "${CMAKE_CURRENT_SOURCE_DIR}/FuzzSeeds/LoadPng"
    DEPS Nexum::Platform)
