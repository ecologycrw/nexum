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

# CA-03: for each packet of the OpenMU XML files, a test generated with the code of the packet (Tools/PacketGen):
# the fixed bytes, the length and the refusal of a shorter packet.
nexum_add_tests(muonline_packet_tests
    SOURCES ${generated_tests}
    DEPS MuOnline)
add_dependencies(muonline_packet_tests muonline_packets)
# Hundreds of packets and test cases in one file pass the number of sections that an MSVC object file allows.
target_compile_options(muonline_packet_tests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

# CA-09: the frame of the foundation screen never asks for memory (INV-13). It is a test program of its own,
# because the allocation counter replaces the global allocation functions of C++ for the whole program that
# links it (tests/architecture/frame_allocations).
nexum_add_tests(muonline_frame_allocation_tests
    SOURCES FrameAllocationTest.cpp
    DEPS MuOnline nexum_test_allocation_counter)
