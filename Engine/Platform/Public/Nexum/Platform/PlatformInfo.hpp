#pragma once

#include <cstdint>

#include "Nexum/Core/Version.hpp"

namespace nexum::platform {

// The operating systems the engine runs on (INV-15).
enum class OperatingSystem : std::uint8_t { Windows, Linux, MacOs };

// The version of the SDL library in use.
[[nodiscard]] core::Version SdlVersion() noexcept;

// The operating system this program was compiled for.
[[nodiscard]] OperatingSystem CurrentOperatingSystem() noexcept;

} // namespace nexum::platform
