#include "Mu/App/ClientOptions.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "Mu/Protocol/Dialect.hpp"
#include "Mu/Protocol/LoginMessage.hpp"

namespace mu::app {

namespace core = nexum::core;

namespace {

constexpr std::string_view kVersion = "--version";
constexpr std::string_view kFrames = "--frames";
constexpr std::string_view kGpuDriver = "--gpu-driver";
constexpr std::string_view kCapture = "--capture";
constexpr std::string_view kConnect = "--connect";
constexpr std::string_view kAccount = "--account";
constexpr std::string_view kServer = "--server";
constexpr std::string_view kCharacter = "--character";
constexpr std::string_view kWorldSeconds = "--world-seconds";

constexpr std::string_view kDevelopmentUsage =
    "Usage: MuClient [--frames N] [--gpu-driver vulkan|direct3d12|metal]\n"
    "                [--connect HOST:PORT --account NAME [--server ID] [--character NAME]\n"
    "                 [--world-seconds N]]\n"
    "       MuClient --capture FILE.png [--gpu-driver vulkan|direct3d12|metal]\n"
    "       MuClient --version\n"
    "With --connect, the password of the account is typed in the terminal, and it does not show.\n";
constexpr std::string_view kPlayerUsage =
    "Usage: MuClient [--frames N] [--gpu-driver vulkan|direct3d12|metal]\n"
    "                [--connect HOST:PORT --account NAME [--server ID] [--character NAME]\n"
    "                 [--world-seconds N]]\n"
    "       MuClient --version\n"
    "With --connect, the password of the account is typed in the terminal, and it does not show.\n";

// The whole number that is all of the text, when it fits in Number: no sign, no spaces, no other characters.
template <typename Number>
[[nodiscard]] std::optional<Number> ParseWhole(std::string_view text) {
    Number value{};
    if (text.empty()) {
        return std::nullopt;
    }
    // from_chars reads between two pointers, so the text needs no null at its end.
    const char* const first = std::to_address(text.begin());
    const char* const last = std::to_address(text.end());
    const auto [end, error] = std::from_chars(first, last, value);
    if (error != std::errc{} || end != last) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] core::Result<std::uint32_t> ParseCount(std::string_view option, std::string_view text) {
    if (const auto count = ParseWhole<std::uint32_t>(text); count && *count > 0) {
        return *count;
    }
    return core::MakeError(std::string(option) + " needs a whole number from 1 to 4294967295, not '" +
                           std::string(text) + "'");
}

[[nodiscard]] core::Result<std::string> ParseCapturePath(std::string_view text) {
    if (text.empty()) {
        return core::MakeError("--capture needs the path of a PNG file");
    }
    return std::string(text);
}

// "HOST:PORT", with a port from 1 to 65535. The host is looked up when the connection opens.
[[nodiscard]] core::Result<ServerAddress> ParseServerAddress(std::string_view text) {
    const std::size_t colon = text.rfind(':');
    if (colon != std::string_view::npos && colon > 0) {
        const std::string_view host = text.substr(0, colon);
        const auto port = ParseWhole<std::uint16_t>(text.substr(colon + 1));
        if (host.find(':') == std::string_view::npos && port && *port > 0) {
            return ServerAddress{.host = std::string(host), .port = *port};
        }
    }
    return core::MakeError(
        "--connect needs HOST:PORT, such as 127.0.0.1:" + std::to_string(protocol::kConnectServerPort) +
        ", not '" + std::string(text) + "'");
}

// The account goes into the login, which holds at most 10 bytes (section 6.6 of the protocol spec).
[[nodiscard]] core::Result<std::string> ParseAccount(std::string_view text) {
    if (!text.empty() && text.size() <= protocol::kLongestUser) {
        return std::string(text);
    }
    return core::MakeError("--account needs a name of 1 to " + std::to_string(protocol::kLongestUser) +
                           " bytes, not '" + std::string(text) + "'");
}

[[nodiscard]] core::Result<std::uint16_t> ParseServerId(std::string_view text) {
    if (const auto id = ParseWhole<std::uint16_t>(text)) {
        return *id;
    }
    return core::MakeError("--server needs a whole number from 0 to 65535, not '" + std::string(text) + "'");
}

[[nodiscard]] core::Result<std::string> ParseCharacter(std::string_view text) {
    if (text.empty()) {
        return core::MakeError("--character needs the name of a character");
    }
    return std::string(text);
}

[[nodiscard]] core::Error GivenTwice(std::string_view option) {
    return core::Error{std::string(option) + " was given twice"};
}

// Keeps the value that `parse()` reads, once: a second value of the same option is refused before it is read.
template <typename Value, typename Parse>
[[nodiscard]] core::Result<void> ReadOnce(std::optional<Value>& field, std::string_view option,
                                          Parse&& parse) {
    if (field) {
        return tl::unexpected(GivenTwice(option));
    }
    auto value = parse();
    if (!value) {
        return tl::unexpected(value.error());
    }
    field = std::move(*value);
    return {};
}

[[nodiscard]] core::Result<void> ReadFrames(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.frameLimit, kFrames, [&] { return ParseCount(kFrames, value); });
}

[[nodiscard]] core::Result<void> ReadGpuDriver(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.gpuBackend, kGpuDriver, [&] { return nexum::render::ParseGpuBackend(value); });
}

[[nodiscard]] core::Result<void> ReadCapture(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.capturePath, kCapture, [&] { return ParseCapturePath(value); });
}

[[nodiscard]] core::Result<void> ReadConnect(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.connectServer, kConnect, [&] { return ParseServerAddress(value); });
}

[[nodiscard]] core::Result<void> ReadAccount(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.account, kAccount, [&] { return ParseAccount(value); });
}

[[nodiscard]] core::Result<void> ReadServer(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.serverId, kServer, [&] { return ParseServerId(value); });
}

[[nodiscard]] core::Result<void> ReadCharacter(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.character, kCharacter, [&] { return ParseCharacter(value); });
}

[[nodiscard]] core::Result<void> ReadWorldSeconds(std::string_view value, ClientOptions& options) {
    return ReadOnce(options.worldSeconds, kWorldSeconds, [&] { return ParseCount(kWorldSeconds, value); });
}

// An option that takes one value, and the function that reads it.
struct ValueOption {
    std::string_view name;
    core::Result<void> (*read)(std::string_view value, ClientOptions& options);
};

constexpr std::array<ValueOption, 8> kValueOptions{{
    {.name = kFrames, .read = ReadFrames},
    {.name = kGpuDriver, .read = ReadGpuDriver},
    {.name = kCapture, .read = ReadCapture},
    {.name = kConnect, .read = ReadConnect},
    {.name = kAccount, .read = ReadAccount},
    {.name = kServer, .read = ReadServer},
    {.name = kCharacter, .read = ReadCharacter},
    {.name = kWorldSeconds, .read = ReadWorldSeconds},
}};

// The option of this build with this name, before its value is read.
[[nodiscard]] core::Result<const ValueOption*> FindOption(std::string_view option, BuildKind build) {
    if (option == kVersion) {
        return core::MakeError("--version comes alone, without other options");
    }
    if (option == kCapture && build == BuildKind::Player) {
        return core::MakeError("--capture exists only in the development build");
    }
    const auto known = std::ranges::find(kValueOptions, option, &ValueOption::name);
    if (known == kValueOptions.end()) {
        return core::MakeError("unknown option '" + std::string(option) + "'");
    }
    return &*known;
}

// The options that only make sense together, or never together.
[[nodiscard]] core::Result<void> CheckTogether(const ClientOptions& options) {
    if (options.capturePath && options.frameLimit) {
        return core::MakeError("--capture draws one scene without a window, so it comes without --frames");
    }
    if (options.capturePath && options.connectServer) {
        return core::MakeError("--capture draws one scene without a window, so it comes without --connect");
    }
    if (options.connectServer && !options.account) {
        return core::MakeError("--connect needs --account, the account of the login");
    }
    if (options.connectServer) {
        return {};
    }
    const std::array<std::pair<bool, std::string_view>, 4> worldOptions{{
        {options.account.has_value(), kAccount},
        {options.serverId.has_value(), kServer},
        {options.character.has_value(), kCharacter},
        {options.worldSeconds.has_value(), kWorldSeconds},
    }};
    for (const auto& [given, name] : worldOptions) {
        if (given) {
            return core::MakeError(std::string(name) + " comes with --connect, the Connect Server");
        }
    }
    return {};
}

} // namespace

std::string_view ClientUsage(BuildKind build) noexcept {
    return build == BuildKind::Development ? kDevelopmentUsage : kPlayerUsage;
}

core::Result<ClientOptions> ParseClientOptions(std::span<const std::string_view> arguments, BuildKind build) {
    ClientOptions options;
    const std::span<const std::string_view> given = arguments.empty() ? arguments : arguments.subspan(1);
    if (given.size() == 1 && given.front() == kVersion) {
        options.showVersion = true;
        return options;
    }
    for (std::size_t index = 0; index < given.size(); ++index) {
        const std::string_view name = given[index];
        auto option = FindOption(name, build);
        if (!option) {
            return tl::unexpected(option.error());
        }
        if (index + 1 == given.size()) {
            return core::MakeError(std::string(name) + " needs a value");
        }
        ++index;
        if (auto read = (*option)->read(given[index], options); !read) {
            return tl::unexpected(read.error());
        }
    }
    if (auto together = CheckTogether(options); !together) {
        return tl::unexpected(together.error());
    }
    return options;
}

core::Result<session::SessionOptions> MakeSessionOptions(const ClientOptions& options, std::string password) {
    const auto refuse = [&password](std::string reason) {
        std::ranges::fill(password, '\0');
        password.clear();
        return core::MakeError(std::move(reason));
    };
    if (!options.connectServer || !options.account) {
        return refuse("the way to the world needs --connect and --account");
    }
    if (password.empty()) {
        return refuse("the password is empty");
    }
    if (password.size() > protocol::kLongestPassword) {
        // The login holds at most 20 bytes of password (section 6.6 of the protocol spec).
        return refuse("the password has more than " + std::to_string(protocol::kLongestPassword) +
                      " bytes, and the login holds no more");
    }
    session::SessionOptions session;
    session.connectServerHost = options.connectServer->host;
    session.connectServerPort = options.connectServer->port;
    session.account = *options.account;
    session.password = std::move(password);
    session.serverId = options.serverId;
    session.character = options.character.value_or(std::string{});
    return session;
}

} // namespace mu::app
