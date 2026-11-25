#include "VisualProof/ProofOptions.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace visualproof {

namespace core = nexum::core;

namespace {

constexpr std::string_view kTolerance = "--tolerance";

[[nodiscard]] core::Result<Tolerance> ParseTolerance(std::string_view text) {
    std::uint8_t levels = 0;
    if (!text.empty()) {
        // from_chars reads between two pointers, so the text needs no null at its end.
        const char* const first = std::to_address(text.begin());
        const char* const last = std::to_address(text.end());
        const auto [end, error] = std::from_chars(first, last, levels);
        if (error == std::errc{} && end == last) {
            return Tolerance{.levels = levels};
        }
    }
    return core::MakeError("--tolerance needs a whole number from 0 to 255, not '" + std::string(text) + "'");
}

// What the command line gave, before it is checked to be complete.
struct GivenArguments {
    std::vector<std::string_view> images;
    std::optional<Tolerance> tolerance;
};

[[nodiscard]] core::Result<void> ReadTolerance(std::span<const std::string_view> given, std::size_t& index,
                                               GivenArguments& read) {
    if (read.tolerance) {
        return core::MakeError("--tolerance was given twice");
    }
    if (index + 1 == given.size()) {
        return core::MakeError("--tolerance needs a value");
    }
    ++index;
    auto tolerance = ParseTolerance(given[index]);
    if (!tolerance) {
        return tl::unexpected(tolerance.error());
    }
    read.tolerance = *tolerance;
    return {};
}

[[nodiscard]] core::Result<GivenArguments> ReadArguments(std::span<const std::string_view> given) {
    GivenArguments read;
    for (std::size_t index = 0; index < given.size(); ++index) {
        const std::string_view argument = given[index];
        if (argument == kTolerance) {
            if (auto tolerance = ReadTolerance(given, index, read); !tolerance) {
                return tl::unexpected(tolerance.error());
            }
        } else if (argument.starts_with("-")) {
            return core::MakeError("unknown option '" + std::string(argument) + "'");
        } else if (read.images.size() == 2) {
            return core::MakeError("unexpected argument '" + std::string(argument) + "'");
        } else {
            read.images.push_back(argument);
        }
    }
    return read;
}

} // namespace

core::Result<ProofOptions> ParseProofOptions(std::span<const std::string_view> arguments) {
    const std::span<const std::string_view> given = arguments.empty() ? arguments : arguments.subspan(1);
    auto read = ReadArguments(given);
    if (!read) {
        return tl::unexpected(read.error());
    }
    if (read->images.size() != 2) {
        return core::MakeError("visualproof needs the capture and the approved image");
    }
    if (!read->tolerance) {
        return core::MakeError(
            "--tolerance is missing: each scene states its own tolerance (docs/VISUAL.md)");
    }
    return ProofOptions{.capture = std::filesystem::path(std::string(read->images[0])),
                        .approved = std::filesystem::path(std::string(read->images[1])),
                        .tolerance = *read->tolerance};
}

} // namespace visualproof
