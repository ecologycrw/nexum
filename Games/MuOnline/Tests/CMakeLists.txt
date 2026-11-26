# The tests of the foundation screen use the real graphics card, like those of Engine/Render: a hardware one on
# the developer's computer and a software one on the CI machines (specs/0000-fundacao/plan.md).
nexum_add_tests(muonline_tests
    SOURCES
        CipherTest.cpp
        ClientCodecTest.cpp
        ClientOptionsTest.cpp
        DialectTest.cpp
        ExtendedAppearanceTest.cpp
        ExtendedItemTest.cpp
        FoundationSceneTest.cpp
        FrameReaderTest.cpp
        FoundationScreenTest.cpp
        MessageQueueTest.cpp
        ModuleIdentityTest.cpp
        PacketExamplesTest.cpp
        PacketLogTest.cpp
        PingTest.cpp
        SessionTest.cpp
        TickReportTextTest.cpp
        TranslatorTest.cpp
        WorldViewTest.cpp
    DEPS MuOnline nexum_test_log)

# CA-03: for each packet of the OpenMU XML files, a test generated with the code of the packet (Tools/PacketGen):
# the fixed bytes, the length and the refusal of a shorter packet. The tests come in files of a few dozen packets,
# each including only the headers of its packets.
nexum_add_tests(muonline_packet_tests
    SOURCES ${generated_tests}
    DEPS MuOnline)
add_dependencies(muonline_packet_tests muonline_packets)

# CA-09: the frame of the foundation screen never asks for memory (INV-13). It is a test program of its own,
# because the allocation counter replaces the global allocation functions of C++ for the whole program that
# links it (tests/architecture/frame_allocations).
nexum_add_tests(muonline_frame_allocation_tests
    SOURCES FrameAllocationTest.cpp
    DEPS MuOnline nexum_test_allocation_counter)

# CA-10: a fuzz target for each reader of outside data: the frames, the encryption, the item, the appearance and
# the packets the client receives. The seeds of each run here as a test on every system.
foreach(fuzzed IN ITEMS "FrameReader;frame_reader" "Cipher;cipher" "ExtendedItem;extended_item"
                        "ExtendedAppearance;extended_appearance" "DialectPackets;dialect_packets")
    list(GET fuzzed 0 reader)
    list(GET fuzzed 1 target)
    nexum_add_fuzz_target(muonline_${target}
        SOURCES ${reader}FuzzTest.cpp
        SEEDS "${CMAKE_CURRENT_SOURCE_DIR}/FuzzSeeds/${reader}"
        DEPS MuOnline)
endforeach()
