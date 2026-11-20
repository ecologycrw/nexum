# The main function shared by every test program (cmake/NexumModule.cmake).
add_library(nexum_test_main OBJECT DoctestMain.cpp)
target_link_libraries(nexum_test_main PUBLIC doctest::doctest)
# With the real standard headers, doctest can print std::string_view and other standard types when a check fails.
target_compile_definitions(nexum_test_main PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
nexum_apply_defaults(nexum_test_main)

nexum_add_tests(nexum_test_main_tests
    SOURCES DoctestMainTest.cpp)
