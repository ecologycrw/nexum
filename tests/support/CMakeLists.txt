# The main function shared by every test program (cmake/NexumModule.cmake).
add_library(nexum_test_main OBJECT DoctestMain.cpp)
target_link_libraries(nexum_test_main PUBLIC doctest::doctest)
# With the real standard headers, doctest can print std::string_view and other standard types when a check fails.
target_compile_definitions(nexum_test_main PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
nexum_apply_defaults(nexum_test_main)

nexum_add_tests(nexum_test_main_tests
    SOURCES DoctestMainTest.cpp)

# A log output in memory, for the tests that check what reaches the log (Nexum/TestSupport/MemoryLogSink.hpp).
# It came from the third copy of the same class in the tests (docs/CONVENTIONS.md, "Regra de três").
add_library(nexum_test_log INTERFACE)
target_include_directories(nexum_test_log INTERFACE Public)
target_link_libraries(nexum_test_log INTERFACE Nexum::Debug)

# The main function of a fuzz target built as a normal test: it runs the target once on each of its seeds
# (cmake/NexumFuzzing.cmake).
add_library(nexum_fuzz_seed_main OBJECT FuzzSeedMain.cpp)
nexum_apply_defaults(nexum_fuzz_seed_main)
