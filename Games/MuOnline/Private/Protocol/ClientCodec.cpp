#include "Mu/Protocol/ClientCodec.hpp"

#include <algorithm>
#include <string>

namespace mu::protocol {

ClientCodec::ClientCodec(Encryption encryption) noexcept : m_encryption(encryption) {}

nexum::core::Result<void> ClientCodec::Receive(std::span<const std::byte> bytes,
                                               nexum::net::MessageSink& sink) {
    nexum::core::Result<void> opened;
    auto framed = m_frames.Push(bytes, [&](std::span<const std::byte> message) {
        if (!opened) {
            return; // the connection is already closing; the rest is dropped
        }
        if (m_encryption == Encryption::None) {
            sink.Deliver(message);
            return;
        }
        auto plain = m_decryptor.Decrypt(message, m_plain);
        if (!plain) {
            opened = tl::unexpected(plain.error());
            return;
        }
        sink.Deliver(std::span<const std::byte>(m_plain).first(*plain));
    });
    if (!framed) {
        return framed;
    }
    return opened;
}

nexum::core::Result<std::size_t> ClientCodec::Send(std::span<const std::byte> message,
                                                   std::span<std::byte> wire) {
    if (message.empty() || message.size() > m_plain.size()) {
        return nexum::core::MakeError("an outgoing message of " + std::to_string(message.size()) + " bytes");
    }
    if (m_encryption == Encryption::None) {
        std::ranges::copy(message, wire.begin());
        return message.size();
    }
    const auto plain = std::span<std::byte>(m_plain).first(message.size());
    std::ranges::copy(message, plain.begin());
    Xor32Encrypt(plain);
    return m_encryptor.Encrypt(plain, wire);
}

std::size_t ClientCodec::WireCapacity(std::size_t messageSize) const noexcept {
    // The largest header (3 bytes), then 11 bytes on the wire for each 8 bytes of the counter and the message
    // (section 4.2). A C1 or a C2 takes only its own size, which is smaller.
    return 3 + (kEncryptedBlockSize * ((messageSize + kPlainBlockSize) / kPlainBlockSize));
}

} // namespace mu::protocol
