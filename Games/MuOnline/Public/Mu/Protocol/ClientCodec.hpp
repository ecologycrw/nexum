#pragma once

// The transformation of bytes of the 106.3 client, which the engine runs on the thread of each connection
// (Nexum/Net/MessageCodec.hpp). Coming in: the framing, then SimpleModulus on C3 and C4. Going out: Xor32 on
// every message, then SimpleModulus on C3 and C4, with the counter. The Connect Server uses only the framing.
// The order comes from the protocol spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md,
// sections 2 and 4.1). Each connection gets a codec of its own, because both counters start at 0 with it.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Mu/Protocol/Cipher.hpp"
#include "Mu/Protocol/FrameReader.hpp"
#include "Nexum/Net/MessageCodec.hpp"

namespace mu::protocol {

enum class Encryption : std::uint8_t {
    None,       // the Connect Server (section 2)
    GameServer, // Xor32 and SimpleModulus (section 4)
};

class ClientCodec final : public nexum::net::MessageCodec {
public:
    explicit ClientCodec(Encryption encryption) noexcept;

    [[nodiscard]] nexum::core::Result<void> Receive(std::span<const std::byte> bytes,
                                                    nexum::net::MessageSink& sink) override;
    [[nodiscard]] nexum::core::Result<std::size_t> Send(std::span<const std::byte> message,
                                                        std::span<std::byte> wire) override;
    [[nodiscard]] std::size_t WireCapacity(std::size_t messageSize) const noexcept override;

private:
    Encryption m_encryption;
    FrameReader m_frames;
    PacketDecryptor m_decryptor{kServerToClientDecryptKey};
    PacketEncryptor m_encryptor{kClientToServerEncryptKey};
    // Room for the largest message, opened or about to be closed, reserved once with the codec (INV-13).
    std::array<std::byte, 0xFFFF> m_plain{};
};

} // namespace mu::protocol
