# CA-03: the C++ generated from Fixture.xml puts every kind of field on the bytes the XML declares.
nexum_add_tests(packetgen_tests
    SOURCES PacketGenTest.cpp
    DEPS MuOnline)
add_dependencies(packetgen_tests packetgen_fixture)
target_include_directories(packetgen_tests PRIVATE "${fixture_include}")

# The script itself: names, sizes, the order of the structures and the errors for definitions it cannot read.
add_test(NAME packetgen_script_tests
    COMMAND "${Python3_EXECUTABLE}" -I -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}" -p "test_*.py")
