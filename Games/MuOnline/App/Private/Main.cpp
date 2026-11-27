#include <cstddef>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "FoundationApp.hpp"
#include "Mu/App/ClientOptions.hpp"
#include "Mu/ModuleIdentity.hpp"
#include "Mu/Session/Session.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Debug/TerminalLogSink.hpp"
#include "Nexum/Platform/Application.hpp"
#include "Nexum/Platform/FailureReports.hpp"
#include "Nexum/Platform/Terminal.hpp"

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
// The exit code of a password that cannot be read or used.
constexpr int kPasswordError = 1;

using OptionalSession = std::optional<mu::session::SessionOptions>;

// The options of the session, with the password typed in the terminal, when the command line goes to the
// world. The password is read before the window opens, and never comes from the command line (CA-12).
[[nodiscard]] nexum::core::Result<OptionalSession> ReadSession(const mu::app::ClientOptions& options) {
    if (!options.Online()) {
        return OptionalSession{};
    }
    auto password = nexum::platform::ReadHiddenLine("Password of the account " + *options.account + ": ");
    if (!password) {
        return nexum::core::MakeError("could not read the password: " + password.error().message);
    }
    auto session = mu::app::MakeSessionOptions(options, std::move(*password));
    if (!session) {
        return tl::unexpected(session.error());
    }
    return OptionalSession{std::move(*session)};
}

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
    auto session = ReadSession(*options);
    if (!session) {
        const std::string message = "MuClient: " + session.error().message + "\n";
        std::fputs(message.c_str(), stderr);
        return kPasswordError;
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
        [&options, &session, &log](std::span<const std::string_view> /*arguments*/) {
            return mu::app::FoundationApp::Create(*options, std::move(*session), log);
        },
        log);
}
