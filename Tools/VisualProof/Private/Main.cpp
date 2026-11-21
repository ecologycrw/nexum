#include <cstddef>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Nexum/Platform/FailureReports.hpp"
#include "VisualProof/ImageComparison.hpp"
#include "VisualProof/ProofOptions.hpp"

namespace {

// The exit codes: the capture was refused, or the command line could not be read.
constexpr int kRefused = 1;
constexpr int kUsageError = 2;

} // namespace

// visualproof CAPTURE.png APPROVED.png --tolerance LEVELS: compares a capture with the image the owner
// approved (docs/VISUAL.md). It ends with 0 when the capture is approved.
int main(int argc, char* argv[]) {
    nexum::platform::WriteFailuresAsTextWhenAsked();
    const std::span<char*> given(argv, static_cast<std::size_t>(argc));
    const std::vector<std::string_view> arguments(given.begin(), given.end());
    const auto options = visualproof::ParseProofOptions(arguments);
    if (!options) {
        const std::string message =
            "visualproof: " + options.error().message + "\n" + std::string(visualproof::kProofUsage);
        std::fputs(message.c_str(), stderr);
        return kUsageError;
    }

    const auto approval = visualproof::CompareFiles(options->capture, options->approved, options->tolerance);
    if (!approval) {
        const std::string message = "visualproof: refused: " + approval.error().message + "\n";
        std::fputs(message.c_str(), stderr);
        return kRefused;
    }
    const std::string message = "visualproof: approved: " + std::to_string(approval->pixels) +
                                " pixels, the largest difference is " +
                                std::to_string(approval->largestDifference) + " of the " +
                                std::to_string(options->tolerance.levels) + " allowed";
    std::puts(message.c_str());
    return 0;
}
