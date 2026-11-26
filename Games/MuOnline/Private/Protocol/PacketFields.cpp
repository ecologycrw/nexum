#include "Mu/Protocol/PacketFields.hpp"

#include <algorithm>
#include <string>

namespace mu::protocol {

namespace {

// C1 and C3 have the same header, and so do C2 and C4 (protocol spec, section 3).
[[nodiscard]] bool SameHeader(std::byte first, HeaderKind kind) noexcept {
    const auto value = std::to_integer<std::uint8_t>(first);
    if (HasShortLength(kind)) {
        return value == static_cast<std::uint8_t>(HeaderKind::C1) ||
               value == static_cast<std::uint8_t>(HeaderKind::C3);
    }
    return value == static_cast<std::uint8_t>(HeaderKind::C2) ||
           value == static_cast<std::uint8_t>(HeaderKind::C4);
}

[[nodiscard]] std::string Hex(std::uint8_t value) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    return {kDigits[value >> 4U], kDigits[value & 0x0FU]};
}

} // namespace

nexum::core::Result<void> WriteText(std::span<std::byte> field, std::string_view text) {
    if (text.size() > field.size()) {
        return nexum::core::MakeError("the text has " + std::to_string(text.size()) +
                                      " bytes, and the field only " + std::to_string(field.size()));
    }
    std::fill(field.begin(), field.end(), std::byte{0});
    std::transform(text.begin(), text.end(), field.begin(),
                   [](char letter) { return static_cast<std::byte>(letter); });
    return {};
}

nexum::core::Result<void> WriteBinary(std::span<std::byte> field, std::span<const std::byte> bytes) {
    if (bytes.size() > field.size()) {
        return nexum::core::MakeError("the data has " + std::to_string(bytes.size()) +
                                      " bytes, and the field only " + std::to_string(field.size()));
    }
    std::fill(field.begin(), field.end(), std::byte{0});
    std::copy(bytes.begin(), bytes.end(), field.begin());
    return {};
}

std::size_t DeclaredLength(std::span<const std::byte> message) noexcept {
    const auto first = std::to_integer<std::uint8_t>(message[0]);
    if (first == static_cast<std::uint8_t>(HeaderKind::C1) ||
        first == static_cast<std::uint8_t>(HeaderKind::C3)) {
        return ReadByte(message, 1);
    }
    return ReadBigEndian<std::uint16_t>(message, 1);
}

nexum::core::Result<void> CheckPacket(std::span<const std::byte> message, const PacketShape& shape) {
    const std::string name(shape.name);
    const std::size_t codeOffset = HeaderSize(shape.header);
    if (message.size() < codeOffset + 1 || !SameHeader(message[0], shape.header)) {
        return nexum::core::MakeError("the message is not a " + name + ": wrong header");
    }
    if (DeclaredLength(message) != message.size()) {
        return nexum::core::MakeError("the " + name + " declares " + std::to_string(DeclaredLength(message)) +
                                      " bytes, but it has " + std::to_string(message.size()));
    }
    if (ReadByte(message, codeOffset) != shape.code) {
        return nexum::core::MakeError("the message is not a " + name + ": code " +
                                      Hex(ReadByte(message, codeOffset)) + " instead of " + Hex(shape.code));
    }
    if (shape.hasSubCode &&
        (message.size() <= codeOffset + 1 || ReadByte(message, codeOffset + 1) != shape.subCode)) {
        return nexum::core::MakeError("the message is not a " + name + ": wrong subcode");
    }
    if (message.size() < shape.minimumLength) {
        return nexum::core::MakeError("the " + name + " needs at least " +
                                      std::to_string(shape.minimumLength) + " bytes, but it has " +
                                      std::to_string(message.size()));
    }
    return {};
}

nexum::core::Result<void> StartPacket(std::span<std::byte> buffer, const PacketShape& shape) {
    const std::string name(shape.name);
    if (buffer.size() < shape.minimumLength || buffer.size() > MaximumLength(shape.header)) {
        return nexum::core::MakeError("a " + name + " has from " + std::to_string(shape.minimumLength) +
                                      " to " + std::to_string(MaximumLength(shape.header)) + " bytes, not " +
                                      std::to_string(buffer.size()));
    }
    std::fill(buffer.begin(), buffer.end(), std::byte{0});
    WriteByte(buffer, 0, static_cast<std::uint8_t>(shape.header));
    if (HasShortLength(shape.header)) {
        WriteByte(buffer, 1, static_cast<std::uint8_t>(buffer.size()));
    } else {
        WriteBigEndian(buffer, 1, static_cast<std::uint16_t>(buffer.size()));
    }
    const std::size_t codeOffset = HeaderSize(shape.header);
    WriteByte(buffer, codeOffset, shape.code);
    if (shape.hasSubCode) {
        WriteByte(buffer, codeOffset + 1, shape.subCode);
    }
    return {};
}

nexum::core::Result<void> CheckStructure(std::span<const std::byte> bytes, std::string_view name,
                                         std::size_t size) {
    if (bytes.size() < size) {
        return nexum::core::MakeError("the " + std::string(name) + " needs " + std::to_string(size) +
                                      " bytes, but only " + std::to_string(bytes.size()) + " are left");
    }
    return {};
}

} // namespace mu::protocol
