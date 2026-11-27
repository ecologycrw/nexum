#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "Mu/ModuleIdentity.hpp"
#include "Mu/Session/Session.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Render/GpuBackend.hpp"

namespace mu::app {

// Where the Connect Server listens.
struct ServerAddress {
    std::string host;
    std::uint16_t port = 0;

    friend bool operator==(const ServerAddress&, const ServerAddress&) = default;
};

// What the command line of MuClient asks for.
struct ClientOptions {
    // --version: write the version and the build kind, and end without opening a window.
    bool showVersion = false;
    // --frames N: end with success after N frames. Empty: run until the window is closed.
    std::optional<std::uint32_t> frameLimit;
    // --gpu-driver NAME: use only this backend (CA-06 of specs/0000-fundacao/spec.md). Empty: the order of
    // the system.
    std::optional<nexum::render::GpuBackend> gpuBackend;
    // --capture FILE: write the proof scene to this PNG file without opening a window, and end (CA-07). Only
    // the development build has it.
    std::optional<std::string> capturePath;

    // The way to the world (specs/0001-rede-ate-o-mundo/spec.md). The password is typed in the terminal,
    // never given on the command line (CA-12).
    // --connect HOST:PORT: the Connect Server. It comes with --account.
    std::optional<ServerAddress> connectServer;
    // --account NAME: the account of the login.
    std::optional<std::string> account;
    // --server ID: the server of the list to choose. Empty: the first of the list.
    std::optional<std::uint16_t> serverId;
    // --character NAME: the character to choose. Empty: the first of the list.
    std::optional<std::string> character;
    // --world-seconds N: end with success after N seconds in the world. Empty: stay until the window is
    // closed.
    std::optional<std::uint32_t> worldSeconds;

    // The recording and the replay of a session (specs/0002-gravacao-e-replay/spec.md, CA-08). Only the
    // development build has them.
    // --record FILE: record the session in the world to this .mucap file. It comes with --connect.
    std::optional<std::string> recordPath;
    // --server-version TEXT: the version of the server, for the header of the recording, with 1 to 64 bytes.
    // It comes with --record. Empty: the header says "unknown".
    std::optional<std::string> serverVersion;
    // --replay FILE: play this .mucap recording without a server. It comes without the options of the way to
    // the world, without --record and without --capture.
    std::optional<std::string> replayPath;

    // True when the client goes to the world.
    [[nodiscard]] bool Online() const noexcept { return connectServer.has_value(); }
};

// How to call MuClient in this kind of build, for the error output.
[[nodiscard]] std::string_view ClientUsage(BuildKind build) noexcept;

// Reads the command line (INV-12). The first argument is the name of the program. Each option comes at most
// once, --version comes alone, the number of frames and of seconds goes from 1 to 4294967295, and --capture,
// which draws without a window, comes without --frames and without --connect. --connect comes with --account,
// and the other options of the way to the world, --record among them, come with --connect. --server-version
// comes with --record, and --replay without the way to the world, --record and --capture. The player build
// refuses --capture, --record, --server-version and --replay (INV-19).
[[nodiscard]] nexum::core::Result<ClientOptions>
ParseClientOptions(std::span<const std::string_view> arguments, BuildKind build);

// The options of the session, from the command line and the password typed in the terminal. An empty password
// and one longer than the login holds are refused here, before any connection, without showing the password
// in the error (CA-12).
[[nodiscard]] nexum::core::Result<session::SessionOptions> MakeSessionOptions(const ClientOptions& options,
                                                                              std::string password);

} // namespace mu::app
