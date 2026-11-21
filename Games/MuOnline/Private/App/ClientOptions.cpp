#include "Mu/App/ClientOptions.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

namespace mu::app {

namespace core = nexum::core;

namespace {

constexpr std::string_view kVersion = "--version";
constexpr std::string_view kFrames = "--frames";
constexpr std::string_view kGpuDriver = "--gpu-driver";

[[nodiscard]] core::Result<std::uint32_t> ParseFrameCount(std::string_view text) {
    std::uint32_t count = 0;
    if (!text.empty()) {
        // from_chars reads between two pointers, so the text needs no null at its end.
        const char* const first = std::to_address(text.begin());
        const char* const last = std::to_address(text.end());
        const auto [end, error] = std::from_chars(first, last, count);
        if (error == std::errc{} && end == last && count > 0) {
            return count;
        }
    }
    return core::MakeError("--frames needs a whole number from 1 to 4294967295, not '" + std::string(text) +
                           "'");
}

[[nodiscard]] core::Error GivenTwice(std::string_view option) {
    return core::Error{std::string(option) + " was given twice"};
}

// Reads the value of an option that takes one.
[[nodiscard]] core::Result<void> ReadValue(std::string_view option, std::string_view value,
                                           ClientOptions& options) {
    if (option == kFrames) {
        if (options.frameLimit) {
            return tl::unexpected(GivenTwice(option));
        }
        auto count = ParseFrameCount(value);
        if (!count) {
            return tl::unexpected(count.error());
        }
        options.frameLimit = *count;
        return {};
    }
    if (options.gpuBackend) {
        return tl::unexpected(GivenTwice(option));
    }
    auto backend = nexum::render::ParseGpuBackend(value);
    if (!backend) {
        return tl::unexpected(backend.error());
    }
    options.gpuBackend = *backend;
    return {};
}

} // namespace

core::Result<ClientOptions> ParseClientOptions(std::span<const std::string_view> arguments) {
    ClientOptions options;
    const std::span<const std::string_view> given = arguments.empty() ? arguments : arguments.subspan(1);
    if (given.size() == 1 && given.front() == kVersion) {
        options.showVersion = true;
        return options;
    }
    for (std::size_t index = 0; index < given.size(); ++index) {
        const std::string_view option = given[index];
        if (option == kVersion) {
            return core::MakeError("--version comes alone, without other options");
        }
        if (option != kFrames && option != kGpuDriver) {
            return core::MakeError("unknown option '" + std::string(option) + "'");
        }
        if (index + 1 == given.size()) {
            return core::MakeError(std::string(option) + " needs a value");
        }
        ++index;
        if (auto read = ReadValue(option, given[index], options); !read) {
            return tl::unexpected(read.error());
        }
    }
    return options;
}

} // namespace mu::app
