#pragma once

// NEXUM_ASSERT(condition) catches bugs: a condition that can only be false if the code is wrong. Expected
// failures, such as bad data from a file or from the network, return a Result instead (INV-14).
//
// In the development build, a false condition writes the condition, the file and the line to the error output
// and ends the program. In the player build, the assert is off: the condition is not even evaluated, so it
// must have no side effect. NEXUM_ASSERTS_ENABLED comes from Engine/Core/CMakeLists.txt.

#ifndef NEXUM_ASSERTS_ENABLED
#error "NEXUM_ASSERTS_ENABLED is missing: link the target to Nexum::Core"
#endif

namespace nexum::core {

// Writes the message of a false NEXUM_ASSERT and ends the program. Called only by the macro.
[[noreturn]] void AssertFailed(const char* condition, const char* file, int line) noexcept;

} // namespace nexum::core

#if NEXUM_ASSERTS_ENABLED
#define NEXUM_ASSERT(condition)                                                                              \
    ((condition) ? static_cast<void>(0) : ::nexum::core::AssertFailed(#condition, __FILE__, __LINE__))
#else
// The condition stays in the code, unevaluated, so its names still count as used and still compile.
#define NEXUM_ASSERT(condition) static_cast<void>(sizeof((condition) ? true : false))
#endif
