#pragma once

// One TCP connection, on a thread of its own (docs/adr/0006-modelo-de-threads.md). The main thread only puts
// messages on the single outgoing queue and takes the events of the incoming queue; it never waits for the
// network (INV-06). The network thread reads and writes the socket and runs the codec of the game, so the
// messages leave in the order of the queue (INV-09). The engine knows nothing of the game: only bytes and the
// codec it was given.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Net/MessageCodec.hpp"

namespace nexum::net {

enum class EventKind : std::uint8_t {
    Connected, // the connection is open
    Message,   // a whole message, already opened by the codec
    Closed,    // the connection is over; the reason says why
};

struct Event {
    EventKind kind = EventKind::Closed;
    std::span<const std::byte> message; // only for Message
    std::string_view reason;            // only for Closed
};

struct ConnectionOptions {
    std::string host;
    std::uint16_t port = 0;
    // The room of each queue, reserved when the connection opens, so the queues never ask for memory
    // afterwards. A message larger than the room closes the connection.
    std::size_t queueBytes = std::size_t{256} * 1024;
};

class Connection {
public:
    // Starts the thread, which looks up the host and connects. The result of the connection arrives as an
    // event: Connected, or Closed with the reason.
    [[nodiscard]] static core::Result<std::unique_ptr<Connection>> Open(const ConnectionOptions& options,
                                                                        std::unique_ptr<MessageCodec> codec);

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;
    // Closes the socket and waits for the thread to end.
    ~Connection();

    // Copies the message to the end of the outgoing queue. An error means the queue has no room left, or the
    // connection is already over; nothing is sent then.
    [[nodiscard]] core::Result<void> Send(std::span<const std::byte> message);

    // Gives every event that arrived since the last call to `handler(const Event&)`, in order, and gives how
    // many. The spans of an event are valid only during the call. It never waits for the network.
    template <typename Handler>
    std::size_t Poll(Handler&& handler) {
        std::span<const std::byte> batch = TakeEvents();
        std::size_t count = 0;
        while (!batch.empty()) {
            const Event event = NextEvent(batch);
            handler(event);
            ++count;
        }
        return count;
    }

    // Asks the network thread to close the connection; a Closed event follows. The messages of the outgoing
    // queue that were not written yet are dropped, not sent.
    void Close() noexcept;

    // What the network thread shares with the main thread; only Connection.cpp knows it. Use Open, not this.
    struct State;
    explicit Connection(std::unique_ptr<State> state) noexcept;

private:
    // The events the network thread wrote since the last call, as records; the next call reuses the room.
    [[nodiscard]] std::span<const std::byte> TakeEvents() noexcept;
    // Reads the record at the start of `batch` and removes it from the batch.
    [[nodiscard]] Event NextEvent(std::span<const std::byte>& batch) const noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::net
