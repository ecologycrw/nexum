#pragma once

// The game's side of a connection. The engine only moves bytes: the game decides where a message ends, how it
// is opened when it arrives and how it is closed before it leaves
// (docs/adr/0010-rede-adaptador-em-camadas.md). Both functions run on the thread of the connection, never on
// the main thread, so the order in which messages are closed is the order of the wire (INV-09).

#include <cstddef>
#include <span>

#include "Nexum/Core/Result.hpp"

namespace nexum::net {

// Receives each whole message the codec found in the bytes, in order. The span is valid only during the call.
class MessageSink {
public:
    virtual ~MessageSink() = default;
    virtual void Deliver(std::span<const std::byte> message) = 0;

protected:
    MessageSink() = default;
    MessageSink(const MessageSink&) = default;
    MessageSink& operator=(const MessageSink&) = default;
    MessageSink(MessageSink&&) = default;
    MessageSink& operator=(MessageSink&&) = default;
};

class MessageCodec {
public:
    virtual ~MessageCodec() = default;

    // Opens the bytes read from the socket into whole messages, given to the sink in order. An error closes
    // the connection, with the error as the reason.
    [[nodiscard]] virtual core::Result<void> Receive(std::span<const std::byte> bytes, MessageSink& sink) = 0;

    // Closes one message into the bytes that go on the wire, written in `wire`, and gives their size. `wire`
    // has at least WireCapacity(message.size()) bytes. An error closes the connection.
    [[nodiscard]] virtual core::Result<std::size_t> Send(std::span<const std::byte> message,
                                                         std::span<std::byte> wire) = 0;

    // The most bytes that a message of this size can take on the wire.
    [[nodiscard]] virtual std::size_t WireCapacity(std::size_t messageSize) const noexcept = 0;

protected:
    MessageCodec() = default;
    MessageCodec(const MessageCodec&) = default;
    MessageCodec& operator=(const MessageCodec&) = default;
    MessageCodec(MessageCodec&&) = default;
    MessageCodec& operator=(MessageCodec&&) = default;
};

} // namespace nexum::net
