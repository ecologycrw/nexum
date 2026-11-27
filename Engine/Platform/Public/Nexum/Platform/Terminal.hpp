#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "Nexum/Core/Result.hpp"

namespace nexum::platform {

// The longest line ReadHiddenLine accepts, in bytes of UTF-8.
inline constexpr std::size_t kLongestHiddenLine = 256;

// Writes `prompt` to the error output and reads one line of the standard input without showing what is typed,
// for a secret such as a password. The end of the line ("\n" or "\r\n") is not part of the text. When the
// input is not a terminal (a pipe or a file), the line is read as it comes, because nothing shows it there.
//
// It reads the operating system directly, without the buffers of the C library, and erases its own copies.
// The text it gives lives in memory of its own from the start, so moving it along never leaves a copy behind;
// the one who keeps it erases it at the end. An input that ends before any character, a line longer than
// kLongestHiddenLine and a terminal that cannot hide what is typed give an error, and nothing is read then.
[[nodiscard]] core::Result<std::string> ReadHiddenLine(std::string_view prompt);

} // namespace nexum::platform
