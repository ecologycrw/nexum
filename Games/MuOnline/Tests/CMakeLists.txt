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

# CA-09: the frame of the foundation screen never asks for memory (INV-13). It is a test program of its own,
# because the allocation counter replaces the global allocation functions of C++ for the whole program that
# links it (tests/architecture/frame_allocations).
nexum_add_tests(muonline_frame_allocation_tests
    SOURCES FrameAllocationTest.cpp
    DEPS MuOnline nexum_test_allocation_counter)
