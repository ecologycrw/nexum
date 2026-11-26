// A connection against a local test server (CA-06). The codec of these tests belongs to no game: each message
// is one byte with the size of the rest, then the rest.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <doctest/doctest.h>

#include "Nexum/Net/Connection.hpp"
#include "Nexum/Net/LoopbackServer.hpp"

namespace {

using nexum::net::Connection;
using nexum::net::EventKind;
using nexum::net::LoopbackServer;

constexpr double kWait = 10.0;

class SizedCodec final : public nexum::net::MessageCodec {
public:
    nexum::core::Result<void> Receive(std::span<const std::byte> bytes,
                                      nexum::net::MessageSink& sink) override {
        m_pending.insert(m_pending.end(), bytes.begin(), bytes.end());
        while (!m_pending.empty()) {
            const auto size = std::to_integer<std::size_t>(m_pending[0]);
            if (size == 0) {
                return nexum::core::MakeError("the test codec refuses a message of size 0");
            }
            if (m_pending.size() < size + 1) {
                break;
            }
            sink.Deliver(std::span<const std::byte>(m_pending).subspan(1, size));
            m_pending.erase(m_pending.begin(), m_pending.begin() + static_cast<std::ptrdiff_t>(size + 1));
        }
        return {};
    }

    nexum::core::Result<std::size_t> Send(std::span<const std::byte> message,
                                          std::span<std::byte> wire) override {
        wire[0] = std::byte{static_cast<std::uint8_t>(message.size())};
        std::copy(message.begin(), message.end(), wire.begin() + 1);
        return message.size() + 1;
    }

    std::size_t WireCapacity(std::size_t messageSize) const noexcept override { return messageSize + 1; }

private:
    std::vector<std::byte> m_pending;
};

struct Seen {
    bool connected = false;
    std::vector<std::vector<std::byte>> messages;
    bool closed = false;
    std::string reason;
};

// Polls the connection, as a main thread would at every frame, until the condition holds or the time is over.
template <typename Condition>
bool PollUntil(Connection& connection, Seen& seen, Condition condition) {
    for (int attempt = 0; attempt < 10'000 && !condition(seen); ++attempt) {
        connection.Poll([&](const nexum::net::Event& event) {
            if (event.kind == EventKind::Connected) {
                seen.connected = true;
            } else if (event.kind == EventKind::Message) {
                seen.messages.emplace_back(event.message.begin(), event.message.end());
            } else {
                seen.closed = true;
                seen.reason = std::string(event.reason);
            }
        });
        if (!condition(seen)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return condition(seen);
}

[[nodiscard]] std::unique_ptr<Connection> Open(std::string host, std::uint16_t port) {
    auto connection = Connection::Open({std::move(host), port}, std::make_unique<SizedCodec>());
    REQUIRE(connection.has_value());
    return std::move(*connection);
}

[[nodiscard]] std::vector<std::byte> Message(std::uint8_t first, std::size_t size) {
    std::vector<std::byte> message(size);
    for (std::size_t index = 0; index < size; ++index) {
        message[index] = std::byte{static_cast<std::uint8_t>(first + index)};
    }
    return message;
}

[[nodiscard]] std::vector<std::byte> Sized(const std::vector<std::byte>& message) {
    std::vector<std::byte> wire{std::byte{static_cast<std::uint8_t>(message.size())}};
    wire.insert(wire.end(), message.begin(), message.end());
    return wire;
}

} // namespace

TEST_CASE("[0001-CA-06] the connection opens and the messages arrive in the order the server sent them") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    auto connection = Open("127.0.0.1", (*server)->Port());
    REQUIRE((*server)->Accept(kWait).has_value());
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.connected; }));

    // Three messages in one write, and a fourth cut in two writes.
    std::vector<std::byte> together;
    for (std::uint8_t index = 0; index < 3; ++index) {
        const auto sized = Sized(Message(static_cast<std::uint8_t>(10 * index), 5));
        together.insert(together.end(), sized.begin(), sized.end());
    }
    REQUIRE((*server)->Write(together).has_value());
    const auto fourth = Sized(Message(200, 40));
    REQUIRE((*server)->Write(std::span<const std::byte>(fourth).first(7)).has_value());
    REQUIRE((*server)->Write(std::span<const std::byte>(fourth).subspan(7)).has_value());

    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.messages.size() == 4; }));
    CHECK(seen.messages[0] == Message(0, 5));
    CHECK(seen.messages[1] == Message(10, 5));
    CHECK(seen.messages[2] == Message(20, 5));
    CHECK(seen.messages[3] == Message(200, 40));
    CHECK_FALSE(seen.closed);
}

TEST_CASE("[0001-CA-06] the messages leave in the order of the single outgoing queue") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    auto connection = Open("127.0.0.1", (*server)->Port());
    REQUIRE((*server)->Accept(kWait).has_value());

    std::vector<std::byte> expected;
    for (std::uint8_t index = 0; index < 200; ++index) {
        const auto message = Message(index, 1 + (index % 30));
        REQUIRE(connection->Send(message).has_value());
        const auto sized = Sized(message);
        expected.insert(expected.end(), sized.begin(), sized.end());
    }
    const auto received = (*server)->Read(expected.size(), kWait);
    REQUIRE(received.has_value());
    CHECK(*received == expected);
}

TEST_CASE("[0001-CA-06] a server that is not there becomes a clear notice in the incoming queue") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    const std::uint16_t port = (*server)->Port();
    (*server)->StopListening();

    auto connection = Open("127.0.0.1", port);
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.closed; }));
    CHECK_FALSE(seen.connected);
    CHECK(seen.reason.starts_with("could not connect to 127.0.0.1:" + std::to_string(port) + ": "));
    CHECK(connection->Send(Message(1, 3)).error().message == "the connection is over");
}

TEST_CASE("[0001-CA-06] a host that does not exist becomes a clear notice in the incoming queue") {
    auto connection = Open("nexum-test.invalid", 9);
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.closed; }));
    CHECK(seen.reason.starts_with("could not find the host nexum-test.invalid: "));
}

TEST_CASE("[0001-CA-06] a connection closed by the server becomes a clear notice in the incoming queue") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    auto connection = Open("127.0.0.1", (*server)->Port());
    REQUIRE((*server)->Accept(kWait).has_value());
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.connected; }));
    REQUIRE((*server)->Write(Sized(Message(1, 2))).has_value());
    (*server)->CloseClient();
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.closed; }));
    CHECK(seen.messages.size() == 1); // what came before the end is still given, in order
    CHECK(seen.reason == "the server closed the connection");
}

TEST_CASE(
    "[0001-CA-06] a connection closed by the client becomes a clear notice, and the server sees the end") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    auto connection = Open("127.0.0.1", (*server)->Port());
    REQUIRE((*server)->Accept(kWait).has_value());
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.connected; }));
    connection->Close();
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.closed; }));
    CHECK(seen.reason == "the client closed the connection");
    CHECK((*server)->WaitForClientToClose(kWait));
}

TEST_CASE("[0001-CA-06] bytes that the codec refuses close the connection, with the reason of the codec") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    auto connection = Open("127.0.0.1", (*server)->Port());
    REQUIRE((*server)->Accept(kWait).has_value());
    REQUIRE((*server)->Write(std::vector<std::byte>{std::byte{0}}).has_value());
    Seen seen;
    REQUIRE(PollUntil(*connection, seen, [](const Seen& state) { return state.closed; }));
    CHECK(seen.reason == "the test codec refuses a message of size 0");
}

TEST_CASE("[0001-CA-06] the outgoing queue refuses a message when it has no room, and polling never waits") {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    nexum::net::ConnectionOptions options{"127.0.0.1", (*server)->Port()};
    options.queueBytes = 64;
    auto opened = Connection::Open(options, std::make_unique<SizedCodec>());
    REQUIRE(opened.has_value());
    auto& connection = **opened;
    // The server never answers: polling comes back with what arrived, never waiting for the network.
    for (int frame = 0; frame < 100; ++frame) {
        CHECK(connection.Poll([](const nexum::net::Event&) {}) <= 1);
    }
    // A message and its size take 4 bytes more than the message in the queue: 60 fit in 64, and 61 do not.
    CHECK(connection.Send(Message(0, 61)).error().message == "the outgoing queue is full");
    CHECK(connection.Send(Message(0, 60)).has_value());
}

TEST_CASE("a connection needs a host, a port and a codec") {
    CHECK(Connection::Open({"", 9}, std::make_unique<SizedCodec>()).error().message ==
          "a connection needs a host and a port");
    CHECK_FALSE(Connection::Open({"127.0.0.1", 0}, std::make_unique<SizedCodec>()).has_value());
    CHECK(Connection::Open({"127.0.0.1", 9}, nullptr).error().message ==
          "a connection needs the codec of the game");
    nexum::net::ConnectionOptions small{"127.0.0.1", 9};
    small.queueBytes = 63;
    CHECK(Connection::Open(small, std::make_unique<SizedCodec>()).error().message ==
          "each queue of a connection needs at least 64 bytes, and 63 were asked for");
}
