#pragma once

// A TCP server on this computer (127.0.0.1) with a single client, driven step by step by whoever owns it:
// accept, read, write and close, each with a time limit. It exists for the tests of connections and of the
// games, which cannot open sockets themselves, because only Engine/Net may use Asio (INV-10). Each call waits
// on the thread that makes it, so it is never used on the main thread of a game.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Nexum/Core/Result.hpp"

namespace nexum::net {

class LoopbackServer {
public:
    // Listens on 127.0.0.1, on a free port.
    [[nodiscard]] static core::Result<std::unique_ptr<LoopbackServer>> Start();

    LoopbackServer(const LoopbackServer&) = delete;
    LoopbackServer& operator=(const LoopbackServer&) = delete;
    LoopbackServer(LoopbackServer&&) = delete;
    LoopbackServer& operator=(LoopbackServer&&) = delete;
    ~LoopbackServer();

    [[nodiscard]] std::uint16_t Port() const noexcept;

    // Waits up to `seconds` for a client to connect.
    [[nodiscard]] core::Result<void> Accept(double seconds);

    // Sends the bytes to the client.
    [[nodiscard]] core::Result<void> Write(std::span<const std::byte> bytes);

    // Waits up to `seconds` until `count` bytes arrived from the client, and gives them. The end of the
    // connection before that is an error that says so.
    [[nodiscard]] core::Result<std::vector<std::byte>> Read(std::size_t count, double seconds);

    // Waits up to `seconds` for the client to close its side; true when it did.
    [[nodiscard]] bool WaitForClientToClose(double seconds);

    // Closes the connection of the client, as a server that goes away would.
    void CloseClient() noexcept;

    // Stops listening: from then on, a new connection to the port is refused.
    void StopListening() noexcept;

    struct State;
    explicit LoopbackServer(std::unique_ptr<State> state) noexcept;

private:
    std::unique_ptr<State> m_state;
};

} // namespace nexum::net
