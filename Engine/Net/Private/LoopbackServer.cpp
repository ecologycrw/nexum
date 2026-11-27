#include "Nexum/Net/LoopbackServer.hpp"

#include <array>
#include <chrono>
#include <string>
#include <utility>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

namespace nexum::net {

struct LoopbackServer::State {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor{io};
    asio::ip::tcp::socket client{io};
};

namespace {

// Asio gives each error twice: in the argument and as the result. Closing or cancelling something that may be
// over already can fail without harm, so that error is dropped here, on purpose.
template <typename Operation>
void Quietly(Operation&& operation) noexcept {
    asio::error_code error;
    const asio::error_code result = operation(error);
    static_cast<void>(result);
}

[[nodiscard]] std::chrono::nanoseconds Limit(double seconds) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(seconds));
}

// Runs the work of the server until `done` or until the time limit; on the limit, cancels what is still
// waiting.
[[nodiscard]] bool RunUntil(asio::io_context& io, const bool& done, double seconds,
                            asio::ip::tcp::socket& socket) {
    io.restart();
    const auto limit = Limit(seconds);
    io.run_for(limit);
    if (!done) {
        Quietly([&socket](asio::error_code& error) { return socket.cancel(error); });
        io.restart();
        io.run(); // the cancelled work ends at once
    }
    return done;
}

} // namespace

core::Result<std::unique_ptr<LoopbackServer>> LoopbackServer::Start() {
    auto state = std::make_unique<State>();
    asio::error_code error;
    const asio::ip::tcp::endpoint local(asio::ip::make_address_v4("127.0.0.1"), 0);
    error = state->acceptor.open(local.protocol(), error);
    if (!error) {
        error = state->acceptor.bind(local, error);
    }
    if (!error) {
        error = state->acceptor.listen(asio::socket_base::max_listen_connections, error);
    }
    if (error) {
        return core::MakeError("could not listen on 127.0.0.1: " + error.message());
    }
    return std::make_unique<LoopbackServer>(std::move(state));
}

LoopbackServer::LoopbackServer(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

LoopbackServer::~LoopbackServer() {
    CloseClient();
    StopListening();
}

std::uint16_t LoopbackServer::Port() const noexcept {
    asio::error_code error;
    const auto endpoint = m_state->acceptor.local_endpoint(error);
    return error ? std::uint16_t{0} : endpoint.port();
}

core::Result<void> LoopbackServer::Accept(double seconds) {
    bool done = false;
    asio::error_code result;
    m_state->acceptor.async_accept(m_state->client, [&](asio::error_code error) {
        result = error;
        done = true;
    });
    m_state->io.restart();
    m_state->io.run_for(Limit(seconds));
    if (!done) {
        Quietly([this](asio::error_code& error) { return m_state->acceptor.cancel(error); });
        m_state->io.restart();
        m_state->io.run();
        return core::MakeError("no client connected in " + std::to_string(seconds) + " s");
    }
    if (result) {
        return core::MakeError("could not accept the client: " + result.message());
    }
    return {};
}

core::Result<void> LoopbackServer::Write(std::span<const std::byte> bytes) {
    asio::error_code error;
    asio::write(m_state->client, asio::buffer(bytes.data(), bytes.size()), error);
    if (error) {
        return core::MakeError("could not write to the client: " + error.message());
    }
    return {};
}

core::Result<std::vector<std::byte>> LoopbackServer::Read(std::size_t count, double seconds) {
    std::vector<std::byte> bytes(count);
    bool done = false;
    asio::error_code result;
    std::size_t received = 0;
    asio::async_read(m_state->client, asio::buffer(bytes.data(), bytes.size()),
                     [&](asio::error_code error, std::size_t size) {
                         result = error;
                         received = size;
                         done = true;
                     });
    if (!RunUntil(m_state->io, done, seconds, m_state->client)) {
        return core::MakeError("only " + std::to_string(received) + " of " + std::to_string(count) +
                               " bytes arrived in " + std::to_string(seconds) + " s");
    }
    if (result) {
        return core::MakeError("the client connection ended after " + std::to_string(received) + " of " +
                               std::to_string(count) + " bytes: " + result.message());
    }
    return bytes;
}

bool LoopbackServer::WaitForClientToClose(double seconds) {
    std::array<std::byte, 1> byte{};
    bool done = false;
    bool closed = false;
    m_state->client.async_read_some(asio::buffer(byte), [&](asio::error_code error, std::size_t /*size*/) {
        closed = static_cast<bool>(error);
        done = true;
    });
    return RunUntil(m_state->io, done, seconds, m_state->client) && closed;
}

void LoopbackServer::CloseClient() noexcept {
    Quietly([this](asio::error_code& error) {
        return m_state->client.shutdown(asio::ip::tcp::socket::shutdown_both, error);
    });
    Quietly([this](asio::error_code& error) { return m_state->client.close(error); });
}

void LoopbackServer::StopListening() noexcept {
    Quietly([this](asio::error_code& error) { return m_state->acceptor.close(error); });
}

} // namespace nexum::net
