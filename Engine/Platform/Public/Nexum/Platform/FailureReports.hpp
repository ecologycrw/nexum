#pragma once

namespace nexum::platform {

// The environment variable with which the test presets ask the programs they run to write their failures as
// text (CMakePresets.json). Any value other than empty or "0" asks for it.
inline constexpr const char* kFailuresAsTextVariable = "NEXUM_FAILURES_AS_TEXT";

// Whether a value of that variable asks for the failures as text. A missing variable is nullptr.
[[nodiscard]] bool AsksForFailuresAsText(const char* value) noexcept;

// On Windows, a failed assertion of the C runtime or an abort opens a dialog that stops the program until
// someone clicks it. After this call they are written to the error output instead, and the program ends at
// once. On the other systems it changes nothing: their failures are already text.
void WriteFailuresAsText() noexcept;

// Calls WriteFailuresAsText when the environment variable asks for it. Each program calls it first thing in
// its main function, before any path that may fail. A program run by hand keeps the dialog, which helps to
// investigate; a program run by a test does not stop the tests.
void WriteFailuresAsTextWhenAsked() noexcept;

} // namespace nexum::platform
