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
endif()
