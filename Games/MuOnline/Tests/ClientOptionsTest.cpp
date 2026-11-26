#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "Mu/App/ClientOptions.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Render/GpuBackend.hpp"

using mu::BuildKind;
using mu::app::ClientOptions;
using mu::app::ClientUsage;
using mu::app::MakeSessionOptions;
using mu::app::ParseClientOptions;
using mu::app::ServerAddress;
using nexum::render::GpuBackend;

namespace {

// The command line of MuClient with these options after the name of the program.
nexum::core::Result<ClientOptions> Parse(std::initializer_list<std::string_view> options,
                                         BuildKind build = BuildKind::Development) {
    std::vector<std::string_view> arguments{"MuClient"};
    arguments.insert(arguments.end(), options.begin(), options.end());
    return ParseClientOptions(arguments, build);
}

template <typename Value>
std::string ErrorOf(const nexum::core::Result<Value>& result) {
    REQUIRE_FALSE(result.has_value());
    return result.error().message;
}

// The way to the world with these options after --connect and --account.
nexum::core::Result<ClientOptions> ParseOnline(std::initializer_list<std::string_view> options) {
    std::vector<std::string_view> arguments{"MuClient", "--connect", "127.0.0.1:44406", "--account", "test0"};
    arguments.insert(arguments.end(), options.begin(), options.end());
    return ParseClientOptions(arguments, BuildKind::Development);
}

} // namespace

TEST_CASE("without options, the client runs until the window closes, with the backend order of the system") {
    const auto options = Parse({});
    REQUIRE(options.has_value());
    CHECK_FALSE(options->showVersion);
    CHECK_FALSE(options->frameLimit.has_value());
    CHECK_FALSE(options->gpuBackend.has_value());
}

TEST_CASE("an empty command line, without even the name of the program, gives the same options") {
    const auto options = ParseClientOptions({}, BuildKind::Development);
    REQUIRE(options.has_value());
    CHECK_FALSE(options->frameLimit.has_value());
    CHECK_FALSE(options->capturePath.has_value());
}

TEST_CASE("--version alone asks for the version") {
    const auto options = Parse({"--version"});
    REQUIRE(options.has_value());
    CHECK(options->showVersion);
}

TEST_CASE("--frames takes the number of frames, from 1 to 4294967295") {
    CHECK(Parse({"--frames", "300"})->frameLimit == 300U);
    CHECK(Parse({"--frames", "1"})->frameLimit == 1U);
    CHECK(Parse({"--frames", "4294967295"})->frameLimit == 4294967295U);
}

TEST_CASE("[0000-CA-06] --gpu-driver forces a backend, and an unknown name gives a clear error") {
    CHECK(Parse({"--gpu-driver", "vulkan"})->gpuBackend == GpuBackend::Vulkan);
    CHECK(Parse({"--gpu-driver", "direct3d12"})->gpuBackend == GpuBackend::Direct3D12);
    CHECK(Parse({"--gpu-driver", "metal"})->gpuBackend == GpuBackend::Metal);
    CHECK(ErrorOf(Parse({"--gpu-driver", "glide"})) ==
          "unknown GPU driver 'glide': use vulkan, direct3d12 or metal");
    CHECK(ErrorOf(Parse({"--gpu-driver", "Vulkan"})) ==
          "unknown GPU driver 'Vulkan': use vulkan, direct3d12 or metal");
}

TEST_CASE("the options may come in any order") {
    const auto options = Parse({"--gpu-driver", "direct3d12", "--frames", "5"});
    REQUIRE(options.has_value());
    CHECK(options->frameLimit == 5U);
    CHECK(options->gpuBackend == GpuBackend::Direct3D12);
}

TEST_CASE("--frames refuses anything but a whole number from 1 to 4294967295") {
    for (const std::string_view value :
         {"0", "-1", "+5", "abc", "12x", " 5", "5 ", "", "1.5", "4294967296", "99999999999999999999"}) {
        CAPTURE(value);
        CHECK(ErrorOf(Parse({"--frames", value})) ==
              "--frames needs a whole number from 1 to 4294967295, not '" + std::string(value) + "'");
    }
}

TEST_CASE("an option without its value is refused") {
    CHECK(ErrorOf(Parse({"--frames"})) == "--frames needs a value");
    CHECK(ErrorOf(Parse({"--frames", "3", "--gpu-driver"})) == "--gpu-driver needs a value");
}

TEST_CASE("an option given twice is refused") {
    CHECK(ErrorOf(Parse({"--frames", "3", "--frames", "4"})) == "--frames was given twice");
    CHECK(ErrorOf(Parse({"--gpu-driver", "vulkan", "--gpu-driver", "metal"})) ==
          "--gpu-driver was given twice");
}

TEST_CASE("unknown options are refused, and so is --version with other options") {
    CHECK(ErrorOf(Parse({"--unknown"})) == "unknown option '--unknown'");
    CHECK(ErrorOf(Parse({"--frames=300"})) == "unknown option '--frames=300'");
    CHECK(ErrorOf(Parse({"300"})) == "unknown option '300'");
    CHECK(ErrorOf(Parse({"--version", "--frames", "3"})) == "--version comes alone, without other options");
    CHECK(ErrorOf(Parse({"--version", "--version"})) == "--version comes alone, without other options");
}

TEST_CASE("[0000-CA-07] --capture takes the PNG file of the proof scene, with or without --gpu-driver") {
    const auto options = Parse({"--capture", "scene.png"});
    REQUIRE(options.has_value());
    CHECK(options->capturePath == "scene.png");
    CHECK_FALSE(options->frameLimit.has_value());

    const auto forced = Parse({"--gpu-driver", "vulkan", "--capture", "out/scene.png"});
    REQUIRE(forced.has_value());
    CHECK(forced->capturePath == "out/scene.png");
    CHECK(forced->gpuBackend == GpuBackend::Vulkan);
}

TEST_CASE("--capture draws without a window, so it refuses --frames, an empty path and a second --capture") {
    const std::string withFrames = "--capture draws one scene without a window, so it comes without --frames";
    CHECK(ErrorOf(Parse({"--capture", "a.png", "--frames", "3"})) == withFrames);
    CHECK(ErrorOf(Parse({"--frames", "3", "--capture", "a.png"})) == withFrames);
    CHECK(ErrorOf(Parse({"--capture", ""})) == "--capture needs the path of a PNG file");
    CHECK(ErrorOf(Parse({"--capture"})) == "--capture needs a value");
    CHECK(ErrorOf(Parse({"--capture", "a.png", "--capture", "b.png"})) == "--capture was given twice");
}

TEST_CASE("the player build refuses --capture and reads its other options as before (INV-19)") {
    CHECK(ErrorOf(Parse({"--capture", "scene.png"}, BuildKind::Player)) ==
          "--capture exists only in the development build");
    CHECK(ErrorOf(Parse({"--capture"}, BuildKind::Player)) ==
          "--capture exists only in the development build");
    const auto options = Parse({"--frames", "300", "--gpu-driver", "metal"}, BuildKind::Player);
    REQUIRE(options.has_value());
    CHECK(options->frameLimit == 300U);
    CHECK(options->gpuBackend == GpuBackend::Metal);
}

TEST_CASE("only the usage of the development build shows --capture") {
    CHECK(ClientUsage(BuildKind::Development).find("--capture FILE.png") != std::string_view::npos);
    CHECK(ClientUsage(BuildKind::Player).find("--capture") == std::string_view::npos);
    CHECK(ClientUsage(BuildKind::Player).starts_with("Usage: MuClient [--frames N]"));
}

TEST_CASE(
    "--connect and --account take the way to the world, with the first server and character of each list") {
    const auto options = Parse({"--connect", "127.0.0.1:44406", "--account", "test0"});
    REQUIRE(options.has_value());
    CHECK(options->Online());
    CHECK(options->connectServer == ServerAddress{.host = "127.0.0.1", .port = 44406});
    CHECK(options->account == "test0");
    CHECK_FALSE(options->serverId.has_value());
    CHECK_FALSE(options->character.has_value());
    CHECK_FALSE(options->worldSeconds.has_value());
    CHECK_FALSE(Parse({})->Online());
}

TEST_CASE("the way to the world takes the server, the character and the seconds in the world, in any order") {
    const auto options = Parse({"--world-seconds", "180", "--character", "test0Dk", "--server", "2",
                                "--frames", "10", "--account", "test1", "--connect", "localhost:55901"});
    REQUIRE(options.has_value());
    CHECK(options->connectServer == ServerAddress{.host = "localhost", .port = 55901});
    CHECK(options->account == "test1");
    CHECK(options->serverId == 2U);
    CHECK(options->character == "test0Dk");
    CHECK(options->worldSeconds == 180U);
    CHECK(options->frameLimit == 10U);

    CHECK(ParseOnline({"--server", "0"})->serverId == 0U);
    CHECK(ParseOnline({"--server", "65535"})->serverId == 65535U);
    CHECK(ParseOnline({"--world-seconds", "4294967295"})->worldSeconds == 4294967295U);
}

TEST_CASE("--connect needs HOST:PORT, with a port from 1 to 65535") {
    for (const std::string_view value :
         {"127.0.0.1", "127.0.0.1:", ":44406", "127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:+1",
          "127.0.0.1: 1", "127.0.0.1:44406x", "::1:44406", ""}) {
        CAPTURE(value);
        CHECK(ErrorOf(Parse({"--connect", value, "--account", "test0"})) ==
              "--connect needs HOST:PORT, such as 127.0.0.1:44406, not '" + std::string(value) + "'");
    }
    CHECK(Parse({"--connect", "127.0.0.1:65535", "--account", "test0"})->connectServer->port == 65535U);
    CHECK(Parse({"--connect", "127.0.0.1:1", "--account", "test0"})->connectServer->port == 1U);
}

TEST_CASE("--account takes a name of 1 to 10 bytes, the longest the login holds") {
    CHECK(Parse({"--connect", "127.0.0.1:44406", "--account", "abcdefghij"})->account == "abcdefghij");
    CHECK(ErrorOf(Parse({"--connect", "127.0.0.1:44406", "--account", "abcdefghijk"})) ==
          "--account needs a name of 1 to 10 bytes, not 'abcdefghijk'");
    CHECK(ErrorOf(Parse({"--connect", "127.0.0.1:44406", "--account", ""})) ==
          "--account needs a name of 1 to 10 bytes, not ''");
}

TEST_CASE("--server, --character and --world-seconds refuse what they cannot use") {
    for (const std::string_view value : {"-1", "65536", "x", "", " 1", "1.0"}) {
        CAPTURE(value);
        CHECK(ErrorOf(ParseOnline({"--server", value})) ==
              "--server needs a whole number from 0 to 65535, not '" + std::string(value) + "'");
    }
    CHECK(ErrorOf(ParseOnline({"--character", ""})) == "--character needs the name of a character");
    for (const std::string_view value : {"0", "-5", "4294967296", "1m", ""}) {
        CAPTURE(value);
        CHECK(ErrorOf(ParseOnline({"--world-seconds", value})) ==
              "--world-seconds needs a whole number from 1 to 4294967295, not '" + std::string(value) + "'");
    }
}

TEST_CASE("each option of the way to the world comes once") {
    CHECK(ErrorOf(ParseOnline({"--connect", "127.0.0.1:44406"})) == "--connect was given twice");
    CHECK(ErrorOf(ParseOnline({"--account", "test1"})) == "--account was given twice");
    CHECK(ErrorOf(ParseOnline({"--server", "1", "--server", "2"})) == "--server was given twice");
    CHECK(ErrorOf(ParseOnline({"--character", "a", "--character", "b"})) == "--character was given twice");
    CHECK(ErrorOf(ParseOnline({"--world-seconds", "1", "--world-seconds", "2"})) ==
          "--world-seconds was given twice");
}

TEST_CASE("the options of the way to the world come with --connect, and --connect never with --capture") {
    CHECK(ErrorOf(Parse({"--connect", "127.0.0.1:44406"})) ==
          "--connect needs --account, the account of the login");
    CHECK(ErrorOf(Parse({"--account", "test0"})) == "--account comes with --connect, the Connect Server");
    CHECK(ErrorOf(Parse({"--server", "1"})) == "--server comes with --connect, the Connect Server");
    CHECK(ErrorOf(Parse({"--character", "test0Dk"})) ==
          "--character comes with --connect, the Connect Server");
    CHECK(ErrorOf(Parse({"--world-seconds", "60"})) ==
          "--world-seconds comes with --connect, the Connect Server");
    CHECK(ErrorOf(ParseOnline({"--capture", "scene.png"})) ==
          "--capture draws one scene without a window, so it comes without --connect");
}

TEST_CASE(
    "[0001-CA-12] the password never comes on the command line, and the usage says where it comes from") {
    CHECK(ErrorOf(ParseOnline({"--password", "test0"})) == "unknown option '--password'");
    for (const BuildKind build : {BuildKind::Development, BuildKind::Player}) {
        CHECK(ClientUsage(build).find("--connect HOST:PORT --account NAME") != std::string_view::npos);
        CHECK(ClientUsage(build).find("the password of the account is typed in the terminal") !=
              std::string_view::npos);
    }
}

TEST_CASE(
    "[0001-CA-12] the session gets the options and the password, refused empty or too long without a trace") {
    const auto options = Parse(
        {"--connect", "10.0.0.5:44406", "--account", "test0", "--server", "1", "--character", "test0Dk"});
    REQUIRE(options.has_value());
    const auto session = MakeSessionOptions(*options, "test0");
    REQUIRE(session.has_value());
    CHECK(session->connectServerHost == "10.0.0.5");
    CHECK(session->connectServerPort == 44406);
    CHECK(session->account == "test0");
    CHECK(session->password == "test0");
    CHECK(session->serverId == 1U);
    CHECK(session->character == "test0Dk");

    // Without --server and --character, the session chooses the first of each list.
    const auto defaults = MakeSessionOptions(*ParseOnline({}), "test0");
    REQUIRE(defaults.has_value());
    CHECK_FALSE(defaults->serverId.has_value());
    CHECK(defaults->character.empty());

    // The login holds at most 20 bytes of password (section 6.6 of the protocol spec).
    CHECK(MakeSessionOptions(*options, std::string(20, 'p')).has_value());
    const std::string refused = ErrorOf(MakeSessionOptions(*options, std::string(21, 'q')));
    CHECK(refused == "the password has more than 20 bytes, and the login holds no more");
    CHECK(refused.find('q') == std::string::npos);
    CHECK(ErrorOf(MakeSessionOptions(*options, "")) == "the password is empty");
    CHECK(ErrorOf(MakeSessionOptions(ClientOptions{}, "test0")) ==
          "the way to the world needs --connect and --account");
}
