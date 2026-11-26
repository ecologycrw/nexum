#include "Nexum/Net/Connection.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <asio/connect.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>

namespace nexum::net {

namespace {

// A record of a queue: the kind (1 byte), the size (4 bytes, little-endian) and the bytes.
constexpr std::size_t kEventHeader = 5;
// A message of the outgoing queue: the size (4 bytes, little-endian) and the bytes.
constexpr std::size_t kMessageHeader = 4;
// How long the network thread waits for the socket before it looks at the outgoing queue again. The main
// thread never wakes the network thread, because waking it through Asio would ask for memory at every
// message.
constexpr std::chrono::milliseconds kSendInterval{5};
constexpr std::size_t kReadPiece = std::size_t{16} * 1024;
// The room of a queue must at least hold the notices of the connection and a small message.
constexpr std::size_t kSmallestQueue = 64;

// Asio gives each error twice: in the argument and as the result. Closing or cancelling something that may be
// over already can fail without harm, so that error is dropped here, on purpose.
template <typename Operation>
void Quietly(Operation&& operation) noexcept {
    asio::error_code error;
    const asio::error_code result = operation(error);
    static_cast<void>(result);
}

void AppendSize(std::vector<std::byte>& bytes, std::size_t size) {
    const auto value = static_cast<std::uint32_t>(size);
    for (unsigned index = 0; index < 4; ++index) {
        bytes.push_back(std::byte{static_cast<std::uint8_t>(value >> (8U * index))});
    }
}

[[nodiscard]] std::size_t ReadSize(std::span<const std::byte> bytes) noexcept {
    std::uint32_t value = 0;
    for (unsigned index = 0; index < 4; ++index) {
        value |= std::to_integer<std::uint32_t>(bytes[index]) << (8U * index);
    }
    return value;
}

} // namespace

struct Connection::State {
    State(const ConnectionOptions& options, std::unique_ptr<MessageCodec> givenCodec)
        : host(options.host), port(options.port), capacity(options.queueBytes), codec(std::move(givenCodec)) {
        incomingBack.reserve(capacity);
        incomingFront.reserve(capacity);
        outgoingBack.reserve(capacity);
        outgoingFront.reserve(capacity);
        wire.resize(codec->WireCapacity(capacity));
    }

    std::string host;
    std::uint16_t port;
    std::size_t capacity;
    std::unique_ptr<MessageCodec> codec;

    // Only the network thread touches these.
    asio::io_context io;
    asio::ip::tcp::resolver resolver{io};
    asio::ip::tcp::socket socket{io};
    std::array<std::byte, kReadPiece> readBuffer{};
    std::vector<std::byte> wire;
    std::size_t outgoingOffset = 0;
    bool writing = false;
    bool connected = false;
    bool over = false;

    std::atomic<bool> closeAsked{false};
    std::atomic<bool> stopping{false};
    std::atomic<bool> refusesSends{false};

    // The incoming queue: the network thread writes records to the back; the main thread swaps the two.
    std::mutex incomingMutex;
    std::condition_variable incomingChanged;
    std::vector<std::byte> incomingBack;
    std::vector<std::byte> incomingFront;
    std::string closedReason;
    std::string frontReason;

    // The single outgoing queue: the main thread writes messages to the back; the network thread swaps the
    // two.
    std::mutex outgoingMutex;
    std::vector<std::byte> outgoingBack;
    std::vector<std::byte> outgoingFront;

    std::thread thread;
};

namespace {

using State = Connection::State;

// Writes a record to the incoming queue. When the queue has no room, the network thread waits for the main
// thread to take the events, and stops reading the socket meanwhile.
void Push(State& state, EventKind kind, std::span<const std::byte> bytes) {
    std::unique_lock lock(state.incomingMutex);
    state.incomingChanged.wait(lock, [&] {
        return state.stopping || state.incomingBack.size() + kEventHeader + bytes.size() <= state.capacity;
    });
    if (state.stopping) {
        return;
    }
    state.incomingBack.push_back(std::byte{static_cast<std::uint8_t>(kind)});
    AppendSize(state.incomingBack, bytes.size());
    state.incomingBack.insert(state.incomingBack.end(), bytes.begin(), bytes.end());
}

// Ends the connection once, with the reason, and tells the main thread.
void Finish(State& state, std::string reason) {
    if (state.over) {
        return;
    }
    state.over = true;
    state.refusesSends = true;
    state.resolver.cancel();
    Quietly([&state](asio::error_code& error) { return state.socket.close(error); });
    {
        const std::lock_guard lock(state.incomingMutex);
        state.closedReason = std::move(reason);
    }
    Push(state, EventKind::Closed, {});
}

class IncomingSink final : public MessageSink {
public:
    explicit IncomingSink(State& state) noexcept : m_state(state) {}

    void Deliver(std::span<const std::byte> message) override {
        if (m_state.over) {
            return; // the connection closed in the middle of the bytes; the rest is dropped
        }
        if (kEventHeader + message.size() > m_state.capacity) {
            Finish(m_state, "a message of " + std::to_string(message.size()) +
                                " bytes is larger than the incoming queue");
            return;
        }
        Push(m_state, EventKind::Message, message);
    }

private:
    State& m_state;
};

void StartReading(State& state) {
    state.socket.async_read_some(asio::buffer(state.readBuffer), [&state](asio::error_code error,
                                                                          std::size_t size) {
        if (state.over) {
            return;
        }
        if (error == asio::error::eof) {
            Finish(state, "the server closed the connection");
            return;
        }
        if (error) {
            Finish(state, "the connection failed: " + error.message());
            return;
        }
        IncomingSink sink(state);
        if (auto received = state.codec->Receive(std::span(state.readBuffer).first(size), sink); !received) {
            Finish(state, received.error().message);
            return;
        }
        if (!state.over) {
            StartReading(state);
        }
    });
}

// Takes the next messages of the outgoing queue, in order, closes each one with the codec and writes them in
// one go. Only one write is on the way at a time, so the bytes reach the socket in the order of the queue.
void Flush(State& state) {
    if (!state.connected || state.writing || state.over) {
        return;
    }
    if (state.outgoingOffset == state.outgoingFront.size()) {
        const std::lock_guard lock(state.outgoingMutex);
        state.outgoingFront.clear();
        state.outgoingOffset = 0;
        std::swap(state.outgoingBack, state.outgoingFront);
    }
    std::size_t used = 0;
    while (state.outgoingOffset < state.outgoingFront.size()) {
        const auto record = std::span<const std::byte>(state.outgoingFront).subspan(state.outgoingOffset);
        const std::size_t size = ReadSize(record);
        const auto message = record.subspan(kMessageHeader, size);
        if (state.wire.size() - used < state.codec->WireCapacity(size)) {
            break; // the rest goes in the next write
        }
        auto written = state.codec->Send(message, std::span(state.wire).subspan(used));
        if (!written) {
            Finish(state, written.error().message);
            return;
        }
        used += *written;
        state.outgoingOffset += kMessageHeader + size;
    }
    if (used == 0) {
        return;
    }
    state.writing = true;
    asio::async_write(state.socket, asio::buffer(state.wire.data(), used),
                      [&state](asio::error_code error, std::size_t /*size*/) {
                          state.writing = false;
                          if (error && !state.over) {
                              Finish(state, "could not send: " + error.message());
                          }
                      });
}

void Connect(State& state) {
    state.resolver.async_resolve(
        state.host, std::to_string(state.port),
        [&state](asio::error_code error, const asio::ip::tcp::resolver::results_type& endpoints) {
            if (error) {
                Finish(state, "could not find the host " + state.host + ": " + error.message());
                return;
            }
            asio::async_connect(
                state.socket, endpoints,
                [&state](asio::error_code connectError, const asio::ip::tcp::endpoint& /*peer*/) {
                    if (connectError) {
                        Finish(state, "could not connect to " + state.host + ":" +
                                          std::to_string(state.port) + ": " + connectError.message());
                        return;
                    }
                    state.connected = true;
                    Push(state, EventKind::Connected, {});
                    StartReading(state);
                });
        });
}

// The network thread.
void Run(State& state) {
    Connect(state);
    while (!state.stopping) {
        if (state.closeAsked.exchange(false)) {
            Finish(state, "the client closed the connection");
        }
        Flush(state);
        if (state.over && !state.writing) {
            std::unique_lock lock(state.incomingMutex);
            state.incomingChanged.wait(lock, [&] { return state.stopping.load(); });
            break;
        }
        state.io.run_for(kSendInterval);
        if (state.io.stopped()) {
            state.io.restart();
        }
    }
}

} // namespace

core::Result<std::unique_ptr<Connection>> Connection::Open(const ConnectionOptions& options,
                                                           std::unique_ptr<MessageCodec> codec) {
    if (options.host.empty() || options.port == 0) {
        return core::MakeError("a connection needs a host and a port");
    }
    if (codec == nullptr) {
        return core::MakeError("a connection needs the codec of the game");
    }
    if (options.queueBytes < kSmallestQueue) {
        return core::MakeError("each queue of a connection needs at least " + std::to_string(kSmallestQueue) +
                               " bytes, and " + std::to_string(options.queueBytes) + " were asked for");
    }
    auto state = std::make_unique<State>(options, std::move(codec));
    State& running = *state;
    running.thread = std::thread([&running] { Run(running); });
    return std::make_unique<Connection>(std::move(state));
}

Connection::Connection(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

Connection::~Connection() {
    m_state->stopping = true;
    m_state->io.stop();
    {
        const std::lock_guard lock(m_state->incomingMutex);
    }
    m_state->incomingChanged.notify_all();
    if (m_state->thread.joinable()) {
        m_state->thread.join();
    }
}

core::Result<void> Connection::Send(std::span<const std::byte> message) {
    const std::lock_guard lock(m_state->outgoingMutex);
    if (m_state->refusesSends) {
        return core::MakeError("the connection is over");
    }
    if (m_state->outgoingBack.size() + kMessageHeader + message.size() > m_state->capacity) {
        return core::MakeError("the outgoing queue is full");
    }
    AppendSize(m_state->outgoingBack, message.size());
    m_state->outgoingBack.insert(m_state->outgoingBack.end(), message.begin(), message.end());
    return {};
}

void Connection::Close() noexcept {
    m_state->closeAsked = true;
}

std::span<const std::byte> Connection::TakeEvents() noexcept {
    {
        const std::lock_guard lock(m_state->incomingMutex);
        m_state->incomingFront.clear();
        std::swap(m_state->incomingBack, m_state->incomingFront);
        if (m_state->frontReason.empty() && !m_state->closedReason.empty()) {
            m_state->frontReason = m_state->closedReason; // once, when the connection ends
        }
    }
    m_state->incomingChanged.notify_all();
    return m_state->incomingFront;
}

Event Connection::NextEvent(std::span<const std::byte>& batch) const noexcept {
    Event event;
    event.kind = static_cast<EventKind>(std::to_integer<std::uint8_t>(batch[0]));
    const std::size_t size = ReadSize(batch.subspan(1));
    if (event.kind == EventKind::Message) {
        event.message = batch.subspan(kEventHeader, size);
    }
    if (event.kind == EventKind::Closed) {
        event.reason = m_state->frontReason;
    }
    batch = batch.subspan(kEventHeader + size);
    return event;
}

} // namespace nexum::net
