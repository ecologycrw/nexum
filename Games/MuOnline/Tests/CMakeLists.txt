# The tests of the foundation screen use the real graphics card, like those of Engine/Render: a hardware one on
# the developer's computer and a software one on the CI machines (specs/0000-fundacao/plan.md).
nexum_add_tests(muonline_tests
    SOURCES
        ClientOptionsTest.cpp
        FoundationSceneTest.cpp
        FoundationScreenTest.cpp
        ModuleIdentityTest.cpp
        TickReportTextTest.cpp
    DEPS MuOnline nexum_test_log)
