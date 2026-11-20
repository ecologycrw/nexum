#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <string_view>

#include "Nexum/Core/Result.hpp"

namespace nexum::core {

struct Version {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;

    friend auto operator<=>(const Version&, const Version&) = default;
};

// The version of the engine that was compiled.
[[nodiscard]] Version EngineVersion() noexcept;

// Reads a version written as "major.minor.patch". The text is treated as untrusted input (INV-12).
[[nodiscard]] Result<Version> ParseVersion(std::string_view text);

// Writes the version as "major.minor.patch".
[[nodiscard]] std::string ToString(const Version& version);

} // namespace nexum::core
