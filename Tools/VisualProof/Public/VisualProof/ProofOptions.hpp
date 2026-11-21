#pragma once

#include <filesystem>
#include <span>
#include <string_view>

#include "Nexum/Core/Result.hpp"
#include "VisualProof/ImageComparison.hpp"

namespace visualproof {

// What the command line of visualproof asks for.
struct ProofOptions {
    std::filesystem::path capture;
    std::filesystem::path approved;
    Tolerance tolerance;
};

// How to call visualproof, for the error output.
inline constexpr std::string_view kProofUsage =
    "Usage: visualproof CAPTURE.png APPROVED.png --tolerance LEVELS\n";

// Reads the command line. The first argument is the name of the program. The capture comes before the
// approved image; --tolerance may come anywhere, exactly once, with a whole number from 0 to 255. There is no
// default tolerance, because each scene states its own (docs/VISUAL.md).
[[nodiscard]] nexum::core::Result<ProofOptions>
ParseProofOptions(std::span<const std::string_view> arguments);

} // namespace visualproof
