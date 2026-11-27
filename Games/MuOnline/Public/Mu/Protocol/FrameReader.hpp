#pragma once

// Splits the bytes of a connection into whole messages. TCP hands over pieces of any size: a piece can hold
// several messages, or only part of one. The rules come from the protocol spec of the knowledge center
// (docs/specs/protocolo/protocolo-106-3-m2.md, section 3): the first byte gives the kind of header (C1 to
// C4), and the length, which counts the header, comes in 1 byte (C1, C3) or in 2 bytes big-endian (C2, C4).

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "Mu/Protocol/PacketFields.hpp"
#include "Nexum/Core/Result.hpp"

namespace mu::protocol {

class FrameReader {
public:
    // Reserves room for the largest message once, so reading never asks for memory afterwards (INV-13).
    FrameReader();

    // Gives each whole message to `onMessage(std::span<const std::byte>)`, in order. The span is valid only
    // during the call. An error means that the bytes cannot be a message of the protocol, and the connection
    // must close: there is no way to find the start of the next message again.
    template <typename OnMessage>
    [[nodiscard]] nexum::core::Result<void> Push(std::span<const std::byte> piece, OnMessage&& onMessage) {
        while (!piece.empty()) {
            auto taken = Take(piece);
            if (!taken) {
                return tl::unexpected(taken.error());
            }
            piece = piece.subspan(taken->used);
            if (!taken->message.empty()) {
                onMessage(taken->message);
                Forget(taken->message);
            }
        }
        return {};
    }

    // The bytes of a message that started but did not arrive whole yet.
    [[nodiscard]] std::size_t PendingBytes() const noexcept { return m_pending.size(); }

private:
    struct Taken {
        std::size_t used = 0;
        std::span<const std::byte> message; // empty while the message is not whole
    };

    [[nodiscard]] nexum::core::Result<Taken> Take(std::span<const std::byte> piece);
    void Forget(std::span<const std::byte> message) noexcept;

    std::vector<std::byte> m_pending;
};

} // namespace mu::protocol
