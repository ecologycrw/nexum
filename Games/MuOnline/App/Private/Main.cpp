#include <cstddef>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "FoundationApp.hpp"
#include "Mu/App/ClientOptions.hpp"
#include "Mu/ModuleIdentity.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Debug/TerminalLogSink.hpp"
#include "Nexum/Platform/Application.hpp"
#include "Nexum/Platform/FailureReports.hpp"

#if NEXUM_AUTHORING_ENABLED
#include "ProofCapture.hpp"
#endif

namespace {

#if NEXUM_AUTHORING_ENABLED
constexpr mu::BuildKind kBuildKind = mu::BuildKind::Development;
#else
constexpr mu::BuildKind kBuildKind = mu::BuildKind::Player;
#endif

// The exit code of a command line that cannot be read.
constexpr int kUsageError = 2;

} // namespace

int main(int argc, char* argv[]) {
    nexum::platform::WriteFailuresAsTextWhenAsked();
    const std::span<char*> given(argv, static_cast<std::size_t>(argc));
    const std::vector<std::string_view> arguments(given.begin(), given.end());
    const auto options = mu::app::ParseClientOptions(arguments, kBuildKind);
    if (!options) {
        const std::string message =
            "MuClient: " + options.error().message + "\n" + std::string(mu::app::ClientUsage(kBuildKind));
        std::fputs(message.c_str(), stderr);
        return kUsageError;
    }
    if (options->showVersion) {
        std::puts(mu::DescribeBuild(kBuildKind).c_str());
        return 0;
    }

    nexum::debug::TerminalLogSink terminal;
    nexum::debug::Logger log(terminal);
#if NEXUM_AUTHORING_ENABLED
    // The capture draws without a window, so it does not go through the main callbacks of the platform.
    if (options->capturePath) {
        return mu::app::RunProofCapture(*options, log);
    }
#endif
    return nexum::platform::RunApplication(
        argc, argv,
        [&options, &log](std::span<const std::string_view> /*arguments*/) {
            return mu::app::FoundationApp::Create(*options, log);
        },
        log);
}
