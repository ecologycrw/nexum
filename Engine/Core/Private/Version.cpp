#include "Nexum/Core/Version.hpp"

#include <charconv>
#include <cstddef>
#include <system_error>

namespace nexum::core {

namespace {

// Reads the number at the start of the text and removes it from the text.
[[nodiscard]] Result<std::uint32_t> TakeNumber(std::string_view& text) {
    std::uint32_t number = 0;
    const char* const begin = text.data();
    const auto [next, error] = std::from_chars(begin, begin + text.size(), number);
    if (error != std::errc{}) {
        return MakeError("expected a number in the version, found '" + std::string(text) + "'");
    }
    text.remove_prefix(static_cast<std::size_t>(next - begin));
    return number;
}

// Removes the dot that separates two numbers.
[[nodiscard]] Result<void> TakeDot(std::string_view& text) {
    if (!text.starts_with('.')) {
        return MakeError("expected a dot in the version, found '" + std::string(text) + "'");
    }
    text.remove_prefix(1);
    return {};
}

} // namespace

Version EngineVersion() noexcept {
    return Version{NEXUM_ENGINE_VERSION_MAJOR, NEXUM_ENGINE_VERSION_MINOR, NEXUM_ENGINE_VERSION_PATCH};
}

Result<Version> ParseVersion(std::string_view text) {
    Version version;
    for (std::uint32_t* const part : {&version.major, &version.minor, &version.patch}) {
        if (part != &version.major) {
            if (auto dot = TakeDot(text); !dot) {
                return tl::unexpected(dot.error());
            }
        }
        auto number = TakeNumber(text);
        if (!number) {
            return tl::unexpected(number.error());
        }
        *part = *number;
    }
    if (!text.empty()) {
        return MakeError("unexpected text after the version: '" + std::string(text) + "'");
    }
    return version;
}

std::string ToString(const Version& version) {
    return std::to_string(version.major) + "." + std::to_string(version.minor) + "." +
           std::to_string(version.patch);
}

} // namespace nexum::core
