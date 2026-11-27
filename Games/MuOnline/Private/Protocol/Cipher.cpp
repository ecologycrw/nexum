#include "Mu/Protocol/Cipher.hpp"

#include <algorithm>
#include <string>

#include "Mu/Protocol/PacketFields.hpp"

namespace mu::protocol {

namespace {

// Section 4.2: the xor of the size byte and the start of the checksum.
constexpr std::uint8_t kSizeXor = 0x3D;
constexpr std::uint8_t kChecksumStart = 0xF8;

// Section 4.2: the largest plain C3, whose 185 bytes become 255 on the wire, the most a length byte holds.
constexpr std::size_t kLargestPlainC3 = 185;

// Section 4.3.
constexpr std::array<std::uint8_t, 32> kXor32Key{
    0xAB, 0x11, 0xCD, 0xFE, 0x18, 0x23, 0xC5, 0xA3, 0xCA, 0x33, 0xC1, 0xCC, 0x66, 0x67, 0x21, 0xF3,
    0x32, 0x12, 0x15, 0x35, 0x29, 0xFF, 0xFE, 0x1D, 0x44, 0xEF, 0xCD, 0x41, 0x26, 0x3C, 0x4E, 0x4D};

// Section 4.4.
constexpr std::array<std::uint8_t, 3> kXor3Key{0xFC, 0xCF, 0xAB};

[[nodiscard]] std::uint8_t Byte(std::span<const std::byte> bytes, std::size_t index) noexcept {
    return std::to_integer<std::uint8_t>(bytes[index]);
}

[[nodiscard]] std::uint8_t Checksum(std::span<const std::byte, kPlainBlockSize> plain) noexcept {
    std::uint8_t checksum = kChecksumStart;
    for (const std::byte value : plain) {
        checksum = static_cast<std::uint8_t>(checksum ^ std::to_integer<std::uint8_t>(value));
    }
    return checksum;
}

// The four values of 18 bits in 9 bytes: for each value, the low byte, the middle byte and the 2 top bits, as
// a stream that starts with the highest bit (section 4.2, the packing table).
void Pack(const std::array<std::uint32_t, 4>& values,
          std::span<std::byte, kEncryptedBlockSize> out) noexcept {
    std::array<std::uint32_t, 4> low{};
    std::array<std::uint32_t, 4> middle{};
    std::array<std::uint32_t, 4> top{};
    for (std::size_t index = 0; index < values.size(); ++index) {
        low[index] = values[index] & 0xFFU;
        middle[index] = (values[index] >> 8U) & 0xFFU;
        top[index] = (values[index] >> 16U) & 0x03U;
    }
    const std::array<std::uint32_t, 9> packed{
        low[0],
        middle[0],
        (top[0] << 6U) | (low[1] >> 2U),
        ((low[1] & 0x03U) << 6U) | (middle[1] >> 2U),
        ((middle[1] & 0x03U) << 6U) | (top[1] << 4U) | (low[2] >> 4U),
        ((low[2] & 0x0FU) << 4U) | (middle[2] >> 4U),
        ((middle[2] & 0x0FU) << 4U) | (top[2] << 2U) | (low[3] >> 6U),
        ((low[3] & 0x3FU) << 2U) | (middle[3] >> 6U),
        ((middle[3] & 0x3FU) << 2U) | top[3],
    };
    for (std::size_t index = 0; index < packed.size(); ++index) {
        out[index] = std::byte{static_cast<std::uint8_t>(packed[index])};
    }
}

[[nodiscard]] std::array<std::uint32_t, 4>
Unpack(std::span<const std::byte, kEncryptedBlockSize> in) noexcept {
    std::array<std::uint32_t, 9> b{};
    for (std::size_t index = 0; index < b.size(); ++index) {
        b[index] = std::to_integer<std::uint32_t>(in[index]);
    }
    const auto value = [](std::uint32_t low, std::uint32_t middle, std::uint32_t top) {
        return (low & 0xFFU) | ((middle & 0xFFU) << 8U) | ((top & 0x03U) << 16U);
    };
    return {
        value(b[0], b[1], b[2] >> 6U),
        value(((b[2] & 0x3FU) << 2U) | (b[3] >> 6U), ((b[3] & 0x3FU) << 2U) | (b[4] >> 6U), b[4] >> 4U),
        value(((b[4] & 0x0FU) << 4U) | (b[5] >> 4U), ((b[5] & 0x0FU) << 4U) | (b[6] >> 4U), b[6] >> 2U),
        value(((b[6] & 0x03U) << 6U) | (b[7] >> 2U), ((b[7] & 0x03U) << 6U) | (b[8] >> 2U), b[8]),
    };
}

// The products are taken in 64 bits; with valid data, the values of section 4.2 also fit in 32 bits, and both
// give the same result.
[[nodiscard]] std::uint32_t MultiplyModulo(std::uint32_t value, std::uint32_t multiplier,
                                           std::uint32_t modulus) noexcept {
    return static_cast<std::uint32_t>((std::uint64_t{value} * multiplier) % modulus);
}

void WriteLength(std::span<std::byte> message, std::size_t length) noexcept {
    if (HasShortLength(static_cast<HeaderKind>(Byte(message, 0)))) {
        WriteByte(message, 1, static_cast<std::uint8_t>(length));
    } else {
        WriteBigEndian(message, 1, static_cast<std::uint16_t>(length));
    }
}

[[nodiscard]] bool IsEncrypted(std::uint8_t first) noexcept {
    return first == static_cast<std::uint8_t>(HeaderKind::C3) ||
           first == static_cast<std::uint8_t>(HeaderKind::C4);
}

[[nodiscard]] nexum::core::Result<std::size_t> CopyClear(std::span<const std::byte> message,
                                                         std::span<std::byte> out) {
    if (out.size() < message.size()) {
        return nexum::core::MakeError("no room for the message: " + std::to_string(message.size()) +
                                      " bytes in a buffer of " + std::to_string(out.size()));
    }
    std::copy(message.begin(), message.end(), out.begin());
    return message.size();
}

} // namespace

void EncryptBlock(std::span<const std::byte, kPlainBlockSize> plain, std::size_t realBytes,
                  const SimpleModulusKey& key, std::span<std::byte, kEncryptedBlockSize> encrypted) noexcept {
    std::array<std::uint32_t, 4> values{};
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::uint32_t word =
            Byte(plain, 2 * index) | (std::uint32_t{Byte(plain, (2 * index) + 1)} << 8U);
        const std::uint32_t chained = index == 0 ? 0U : (values[index - 1] & 0xFFFFU);
        values[index] =
            MultiplyModulo(word ^ key.xorKey[index] ^ chained, key.multiplier[index], key.modulus[index]);
    }
    for (std::size_t index = 0; index + 1 < values.size(); ++index) {
        values[index] = values[index] ^ key.xorKey[index] ^ (values[index + 1] & 0xFFFFU);
    }
    Pack(values, encrypted);
    const std::uint8_t checksum = Checksum(plain);
    encrypted[9] = std::byte{static_cast<std::uint8_t>(realBytes ^ kSizeXor ^ checksum)};
    encrypted[10] = std::byte{checksum};
}

nexum::core::Result<std::size_t> DecryptBlock(std::span<const std::byte, kEncryptedBlockSize> encrypted,
                                              const SimpleModulusKey& key,
                                              std::span<std::byte, kPlainBlockSize> plain) {
    auto values = Unpack(encrypted);
    for (std::size_t index = values.size() - 1; index > 0; --index) {
        values[index - 1] = values[index - 1] ^ key.xorKey[index - 1] ^ (values[index] & 0xFFFFU);
    }
    for (std::size_t index = 0; index < values.size(); ++index) {
        std::uint32_t word =
            key.xorKey[index] ^ MultiplyModulo(values[index], key.multiplier[index], key.modulus[index]);
        if (index > 0) {
            word ^= values[index - 1] & 0xFFFFU;
        }
        plain[2 * index] = std::byte{static_cast<std::uint8_t>(word)};
        plain[(2 * index) + 1] = std::byte{static_cast<std::uint8_t>(word >> 8U)};
    }
    const std::uint8_t checksum = Byte(encrypted, 10);
    const auto realBytes = static_cast<std::size_t>(Byte(encrypted, 9) ^ checksum ^ kSizeXor);
    if (realBytes > kPlainBlockSize) {
        return nexum::core::MakeError("a block says it has " + std::to_string(realBytes) +
                                      " bytes, more than 8");
    }
    // The checksum covers the 8 bytes, the padding included: the original client leaves garbage there.
    if (Checksum(plain) != checksum) {
        return nexum::core::MakeError("the checksum of a block is wrong");
    }
    return realBytes;
}

std::size_t EncryptedLength(std::size_t headerSize, std::size_t plainLength) noexcept {
    const std::size_t stream = plainLength - headerSize + 1; // the counter, then the message from its code on
    return headerSize + (kEncryptedBlockSize * ((stream + kPlainBlockSize - 1) / kPlainBlockSize));
}

nexum::core::Result<std::size_t> PacketEncryptor::Encrypt(std::span<const std::byte> message,
                                                          std::span<std::byte> wire) {
    if (message.empty()) {
        return nexum::core::MakeError("an empty message cannot be encrypted");
    }
    const std::uint8_t first = Byte(message, 0);
    if (!IsEncrypted(first)) {
        return CopyClear(message, wire);
    }
    const std::size_t headerSize = HeaderSize(static_cast<HeaderKind>(first));
    if (message.size() <= headerSize) {
        return nexum::core::MakeError("a message of " + std::to_string(message.size()) +
                                      " bytes has no code after its header");
    }
    if (first == static_cast<std::uint8_t>(HeaderKind::C3) && message.size() > kLargestPlainC3) {
        return nexum::core::MakeError("a C3 of " + std::to_string(message.size()) +
                                      " bytes does not fit on the wire: the most is 185");
    }
    const std::size_t length = EncryptedLength(headerSize, message.size());
    if (wire.size() < length || length > MaximumLength(static_cast<HeaderKind>(first))) {
        return nexum::core::MakeError("no room for the encrypted message of " + std::to_string(length) +
                                      " bytes");
    }
    const std::size_t streamLength = message.size() - headerSize + 1;
    const auto streamByte = [&](std::size_t index) {
        return index == 0 ? std::byte{m_counter} : message[headerSize + index - 1];
    };
    wire[0] = message[0];
    WriteLength(wire, length);
    std::size_t out = headerSize;
    for (std::size_t start = 0; start < streamLength; start += kPlainBlockSize) {
        std::array<std::byte, kPlainBlockSize> block{};
        const std::size_t realBytes = std::min(kPlainBlockSize, streamLength - start);
        for (std::size_t index = 0; index < realBytes; ++index) {
            block[index] = streamByte(start + index);
        }
        EncryptBlock(block, realBytes, m_key, wire.subspan(out).first<kEncryptedBlockSize>());
        out += kEncryptedBlockSize;
    }
    m_counter = static_cast<std::uint8_t>(m_counter + 1);
    return length;
}

nexum::core::Result<std::size_t> PacketDecryptor::Decrypt(std::span<const std::byte> wire,
                                                          std::span<std::byte> plain) {
    if (wire.empty()) {
        return nexum::core::MakeError("an empty message cannot be decrypted");
    }
    const std::uint8_t first = Byte(wire, 0);
    if (!IsEncrypted(first)) {
        return CopyClear(wire, plain);
    }
    const std::size_t headerSize = HeaderSize(static_cast<HeaderKind>(first));
    const std::size_t body = wire.size() - std::min(wire.size(), headerSize);
    if (body == 0 || body % kEncryptedBlockSize != 0) {
        return nexum::core::MakeError("an encrypted message of " + std::to_string(wire.size()) +
                                      " bytes is not made of blocks of 11");
    }
    if (plain.size() < wire.size()) {
        return nexum::core::MakeError("no room for the decrypted message");
    }
    // The stream (the counter, then the message from its code on) is written right after the header, one byte
    // before its place; the counter is checked and then the bytes move one place back.
    std::size_t streamLength = 0;
    for (std::size_t start = headerSize; start < wire.size(); start += kEncryptedBlockSize) {
        std::array<std::byte, kPlainBlockSize> block{};
        auto realBytes = DecryptBlock(wire.subspan(start).first<kEncryptedBlockSize>(), m_key, block);
        if (!realBytes) {
            return tl::unexpected(realBytes.error());
        }
        std::copy_n(block.begin(), *realBytes,
                    plain.begin() + static_cast<std::ptrdiff_t>(headerSize + streamLength));
        streamLength += *realBytes;
    }
    if (streamLength < 2) {
        return nexum::core::MakeError("an encrypted message has no counter and code");
    }
    const std::uint8_t counter = std::to_integer<std::uint8_t>(plain[headerSize]);
    if (counter != m_counter) {
        return nexum::core::MakeError("the counter of the message is " + std::to_string(counter) + ", not " +
                                      std::to_string(m_counter));
    }
    const auto streamStart = plain.begin() + static_cast<std::ptrdiff_t>(headerSize);
    std::copy(streamStart + 1, streamStart + static_cast<std::ptrdiff_t>(streamLength), streamStart);
    const std::size_t length = headerSize + streamLength - 1;
    plain[0] = wire[0];
    WriteLength(plain, length);
    m_counter = static_cast<std::uint8_t>(m_counter + 1);
    return length;
}

void Xor32Encrypt(std::span<std::byte> message) noexcept {
    if (message.empty()) {
        return;
    }
    const std::size_t headerSize =
        HeaderSize(static_cast<HeaderKind>(std::to_integer<std::uint8_t>(message[0])));
    for (std::size_t index = headerSize + 1; index < message.size(); ++index) {
        message[index] = message[index] ^ message[index - 1] ^ std::byte{kXor32Key[index % kXor32Key.size()]};
    }
}

void Xor32Decrypt(std::span<std::byte> message) noexcept {
    if (message.empty()) {
        return;
    }
    const std::size_t headerSize =
        HeaderSize(static_cast<HeaderKind>(std::to_integer<std::uint8_t>(message[0])));
    for (std::size_t index = message.size() - 1; index > headerSize; --index) {
        message[index] = message[index] ^ message[index - 1] ^ std::byte{kXor32Key[index % kXor32Key.size()]};
    }
}

void Xor3(std::span<std::byte> field) noexcept {
    for (std::size_t index = 0; index < field.size(); ++index) {
        field[index] = field[index] ^ std::byte{kXor3Key[index % kXor3Key.size()]};
    }
}

} // namespace mu::protocol
