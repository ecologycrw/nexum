#include "Mu/Protocol/ExtendedItem.hpp"

#include <algorithm>
#include <cstdint>

namespace mu::protocol {

namespace {

// The flags of byte 4 (section 7.1).
constexpr std::uint8_t kOption = 0x01;
constexpr std::uint8_t kLuck = 0x02;
constexpr std::uint8_t kSkill = 0x04;
constexpr std::uint8_t kExcellent = 0x08;
constexpr std::uint8_t kAncient = 0x10;
constexpr std::uint8_t kHarmony = 0x20;
constexpr std::uint8_t kOption380 = 0x40;
constexpr std::uint8_t kSockets = 0x80;

constexpr std::size_t kFixedBytes = 5;

// A byte past the end reads as 0 (section 7.1, "Ao ler").
[[nodiscard]] std::uint8_t ByteAt(std::span<const std::byte> bytes, std::size_t index) noexcept {
    return index < bytes.size() ? std::to_integer<std::uint8_t>(bytes[index]) : std::uint8_t{0};
}

[[nodiscard]] std::uint8_t High(std::uint8_t value) noexcept {
    return static_cast<std::uint8_t>(value >> 4U);
}
[[nodiscard]] std::uint8_t Low(std::uint8_t value) noexcept {
    return static_cast<std::uint8_t>(value & 0x0FU);
}

// Reads the sockets at `next`: the first byte has the bonus and the count; one byte per socket follows. The
// count uses the whole nibble for the size, as the spec says, and the item keeps every socket that is there.
[[nodiscard]] std::size_t ReadSockets(std::span<const std::byte> bytes, std::size_t next,
                                      items::Item& item) noexcept {
    const std::uint8_t header = ByteAt(bytes, next);
    item.socketBonus = High(header);
    item.socketCount = Low(header);
    for (std::size_t index = 0; index < item.socketCount; ++index) {
        item.sockets[index] = ByteAt(bytes, next + 1 + index);
    }
    return next + 1 + item.socketCount;
}

} // namespace

ExtendedItem ReadExtendedItem(std::span<const std::byte> bytes) noexcept {
    items::Item item;
    const std::uint8_t first = ByteAt(bytes, 0);
    item.group = High(first);
    item.number = static_cast<std::uint16_t>((Low(first) << 8U) | ByteAt(bytes, 1));
    item.level = ByteAt(bytes, 2);
    item.durability = ByteAt(bytes, 3);
    const std::uint8_t flags = ByteAt(bytes, 4);
    item.luck = (flags & kLuck) != 0;
    item.skill = (flags & kSkill) != 0;
    item.option380 = (flags & kOption380) != 0;

    // The optional parts, in this fixed order: option, excellent, ancient, harmony and sockets.
    std::size_t next = kFixedBytes;
    if ((flags & kOption) != 0) {
        item.hasOption = true;
        item.optionType = High(ByteAt(bytes, next));
        item.optionLevel = Low(ByteAt(bytes, next));
        ++next;
    }
    if ((flags & kExcellent) != 0) {
        item.excellent = ByteAt(bytes, next);
        ++next;
    }
    if ((flags & kAncient) != 0) {
        item.hasAncient = true;
        item.ancientSet = Low(ByteAt(bytes, next));
        item.ancientBonusLevel = High(ByteAt(bytes, next));
        ++next;
    }
    if ((flags & kHarmony) != 0) {
        item.hasHarmony = true;
        item.harmonyNumber = High(ByteAt(bytes, next));
        item.harmonyLevel = Low(ByteAt(bytes, next));
        ++next;
    }
    if ((flags & kSockets) != 0) {
        item.hasSockets = true;
        next = ReadSockets(bytes, next, item);
    }
    return ExtendedItem{item, std::min(next, bytes.size())};
}

} // namespace mu::protocol
