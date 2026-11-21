# The allocation counter of CA-09 (specs/0000-fundacao/spec.md). It replaces the global allocation functions of
# C++ for the whole program that links it, which the cpp-rules check allows only in this folder. Only a test
# program of its own links it, never a module. It is an object library, so its replacements always reach the
# program.
add_library(nexum_test_allocation_counter OBJECT AllocationCounter.cpp)
target_include_directories(nexum_test_allocation_counter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
nexum_apply_defaults(nexum_test_allocation_counter)
