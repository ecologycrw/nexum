// Fuzz target of the encryption (cmake/NexumFuzzing.cmake): any bytes, taken as a message from the server,
// give a plain message or an error, never a crash. Taken as a whole plain message, they come back the same
// after SimpleModulus and after Xor32, there and back.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

#include "Mu/Protocol/Cipher.hpp"
#include "Mu/Protocol/PacketFields.hpp"

namespace {

namespace protocol = mu::protocol;

[[nodiscard]] bool IsWholeMessage(std::span<const std::byte> bytes) noexcept {
    if (bytes.empty()) {
        return false;
    }
    const auto first = std::to_integer<std::uint8_t>(bytes[0]);
    if (first < 0xC1 || first > 0xC4) {
        return false;
    }
    const auto headerSize = protocol::HeaderSize(static_cast<protocol::HeaderKind>(first));
    return bytes.size() > headerSize && protocol::DeclaredLength(bytes) == bytes.size();
}

void DecryptAnything(std::span<const std::byte> bytes) {
    protocol::PacketDecryptor decryptor(protocol::kServerToClientDecryptKey);
    std::vector<std::byte> plain(bytes.size());
    static_cast<void>(decryptor.Decrypt(bytes, plain));
}

void EncryptAndDecrypt(std::span<const std::byte> message) {
    const auto first = std::to_integer<std::uint8_t>(message[0]);
    const auto headerSize = protocol::HeaderSize(static_cast<protocol::HeaderKind>(first));
    protocol::PacketEncryptor encryptor(protocol::kServerToClientEncryptKey);
    std::vector<std::byte> wire(
        std::max(message.size(), protocol::EncryptedLength(headerSize, message.size())));
    const auto written = encryptor.Encrypt(message, wire);
    if (!written) {
        return; // a C3 too large for its length byte
    }
    protocol::PacketDecryptor decryptor(protocol::kServerToClientDecryptKey);
    std::vector<std::byte> plain(*written);
    const auto read = decryptor.Decrypt(std::span<const std::byte>(wire).first(*written), plain);
    if (!read || !std::equal(message.begin(), message.end(), plain.begin(), plain.begin() + *read)) {
        std::abort();
    }
}

void Xor32ThereAndBack(std::span<const std::byte> message) {
    std::vector<std::byte> copy(message.begin(), message.end());
    protocol::Xor32Encrypt(copy);
    protocol::Xor32Decrypt(copy);
    if (!std::equal(message.begin(), message.end(), copy.begin(), copy.end())) {
        std::abort();
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    DecryptAnything(bytes);
    if (IsWholeMessage(bytes)) {
        EncryptAndDecrypt(bytes);
        Xor32ThereAndBack(bytes);
    }
    return 0;
}
