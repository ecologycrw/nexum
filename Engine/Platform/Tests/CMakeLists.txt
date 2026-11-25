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
