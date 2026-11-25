# Programs with deliberate bugs. The tests pass only when the sanitizers catch them.
add_executable(sanitizer_canary_use_after_free UseAfterFree.cpp)
add_test(NAME sanitizers_catch_use_after_free COMMAND sanitizer_canary_use_after_free)
set_tests_properties(sanitizers_catch_use_after_free PROPERTIES
    PASS_REGULAR_EXPRESSION "AddressSanitizer: heap-use-after-free")

if(NOT MSVC)
    add_executable(sanitizer_canary_signed_overflow SignedOverflow.cpp)
    add_test(NAME sanitizers_catch_signed_overflow COMMAND sanitizer_canary_signed_overflow)
    set_tests_properties(sanitizers_catch_signed_overflow PROPERTIES
        PASS_REGULAR_EXPRESSION "runtime error: signed integer overflow")

    # The leak check is on, in the same conditions as the other tests: the test launcher keeps the Vulkan driver
    # loaded and gives the list of leak exceptions (cmake/NexumSanitizers.cmake). MSVC has no leak check.
    add_executable(sanitizer_canary_memory_leak MemoryLeak.cpp)
    add_test(NAME sanitizers_catch_a_memory_leak COMMAND sanitizer_canary_memory_leak)
    set_tests_properties(sanitizers_catch_a_memory_leak PROPERTIES
        PASS_REGULAR_EXPRESSION "Direct leak of 64 byte\\(s\\) in 1 object\\(s\\)")
endif()
