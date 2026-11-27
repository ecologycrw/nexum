#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#include "Nexum/Core/Result.hpp"

namespace nexum::platform {

// Reads the whole content of a file, as it is on the disk. The size of the file is checked first: a file
// larger than limit bytes is refused before any of its content is read and before any memory is asked for it,
// so a file from outside cannot ask for more memory than the caller allows (INV-12). The reading waits for
// the disk, so it never runs inside the frame (INV-06).
[[nodiscard]] core::Result<std::vector<std::byte>> ReadFileBytes(const std::filesystem::path& path,
                                                                 std::size_t limit);

// Writes the bytes as the whole content of a file, replacing the file if it exists. The answer comes after
// the file is closed, so a disk that fills up is an error too. A failure may leave the file cut. The writing
// waits for the disk, so it never runs inside the frame (INV-06).
[[nodiscard]] core::Result<void> WriteFileBytes(const std::filesystem::path& path,
                                                std::span<const std::byte> bytes);

} // namespace nexum::platform
