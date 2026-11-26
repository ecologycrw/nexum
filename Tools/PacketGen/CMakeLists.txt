# packetgen (docs/adr/0034-packetgen-em-python-no-build.md): the Python script in Private/ turns the packet
# definitions of OpenMU into C++. cmake/NexumPacketGen.cmake runs it at every build, also in the player build,
# for the MU module (Games/MuOnline/CMakeLists.txt). This folder only holds the tests of the generator, which, like
# every tool, exist only in the development build (INV-02).

# The code generated from Fixture.xml, a file written for the tests, with every kind of field.
nexum_generate_packets(
    XML "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Fixture.xml"
    NAMESPACE packetgen::fixture
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Generated"
    INCLUDE_DIR "PacketGen/Fixture"
    HEADERS_VARIABLE fixture_headers)
set(fixture_include "${CMAKE_CURRENT_BINARY_DIR}/Generated")
# The rule of a generated file belongs to a target of this folder; the tests in Tests/ wait for it.
add_custom_target(packetgen_fixture DEPENDS ${fixture_headers})

add_subdirectory(Tests)
