#pragma once

// The packets as the log shows them: the direction, the size and the bytes in hexadecimal. The user and the
// password of a login sent never reach the log, not even with Xor3 on them, because its key is public
// (docs/specs/protocolo/protocolo-106-3-m2.md, sections 4.4 and 6.6): their bytes are shown as "**".

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace mu::protocol {

enum class PacketDirection : std::uint8_t { Sent, Received };

// The most bytes of a packet that the log shows; the rest becomes a count.
inline constexpr std::size_t kLoggedPacketBytes = 64;

// "sent 60 bytes: C3 3C F1 01 ** ** ...". It asks for memory, so it is meant for the log only, after the log
// says that the level of the packets is on.
[[nodiscard]] std::string FormatPacket(PacketDirection direction, std::span<const std::byte> message);

} // namespace mu::protocol
