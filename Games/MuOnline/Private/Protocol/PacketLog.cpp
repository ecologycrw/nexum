#include "Mu/Protocol/PacketLog.hpp"

#include <algorithm>
#include <string_view>

#include "Mu/Protocol/Dialect.hpp"

namespace mu::protocol {

namespace {

// The user (10 bytes from byte 4) and the password (20 bytes from byte 14) of the login (section 6.6).
constexpr std::size_t kCredentialsStart = 4;
constexpr std::size_t kCredentialsEnd = 34;

// Every login variant has the code F1 01, so the credentials of any of them are hidden.
[[nodiscard]] bool HasCredentials(PacketDirection direction, std::span<const std::byte> message) noexcept {
    return direction == PacketDirection::Sent && IsPacket(message, dialect::Login::kShape);
}

void AppendHex(std::string& text, std::byte value) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    const auto number = std::to_integer<std::uint8_t>(value);
    text += kDigits[number >> 4U];
    text += kDigits[number & 0x0FU];
}

} // namespace

std::string FormatPacket(PacketDirection direction, std::span<const std::byte> message) {
    const bool hidden = HasCredentials(direction, message);
    const std::size_t shown = std::min(message.size(), kLoggedPacketBytes);
    std::string text = direction == PacketDirection::Sent ? "sent " : "received ";
    text += std::to_string(message.size()) + " bytes:";
    for (std::size_t index = 0; index < shown; ++index) {
        text += ' ';
        if (hidden && index >= kCredentialsStart && index < kCredentialsEnd) {
            text += "**";
        } else {
            AppendHex(text, message[index]);
        }
    }
    if (shown < message.size()) {
        text += " ... (" + std::to_string(message.size() - shown) + " more)";
    }
    return text;
}

} // namespace mu::protocol
