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
    const std::span<char*> given(argv, static_cast<std::size_t>(argc));
    const std::vector<std::string_view> arguments(given.begin(), given.end());
    const auto options = mu::app::ParseClientOptions(arguments);
    if (!options) {
        const std::string message =
            "MuClient: " + options.error().message + "\n" + std::string(mu::app::kClientUsage);
        std::fputs(message.c_str(), stderr);
        return kUsageError;
    }
    if (options->showVersion) {
        std::puts(mu::DescribeBuild(kBuildKind).c_str());
        return 0;
    }

    nexum::debug::TerminalLogSink terminal;
    nexum::debug::Logger log(terminal);
    return nexum::platform::RunApplication(
        argc, argv,
        [&options, &log](std::span<const std::string_view> /*arguments*/) {
            return mu::app::FoundationApp::Create(*options, log);
        },
        log);
}
