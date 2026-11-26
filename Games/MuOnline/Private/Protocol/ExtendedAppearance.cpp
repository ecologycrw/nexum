#include "Mu/Protocol/ExtendedAppearance.hpp"

#include <cstdint>

namespace mu::protocol {

namespace {

// The marks of byte 1 (section 7.2): the pose in bits 0 to 2, the full ancient set and the game master.
constexpr std::uint8_t kPoseMask = 0x07;
constexpr std::uint8_t kFullAncientSet = 0x10;
constexpr std::uint8_t kGameMaster = 0x20;

// The third byte of an item with glow: the glow in the high nibble, then the marks.
constexpr std::uint8_t kExcellent = 0x08;
constexpr std::uint8_t kAncient = 0x04;

constexpr std::size_t kFirstWorn = 2;
constexpr std::size_t kWingsAt = 23;
constexpr std::size_t kPetAt = 25;

// The Fenrir is the pet of group 13, number 37 (finding 2026-11-21-aparencia-estendida.md, section 4). Its
// variant is an OR on bits 1 to 3 of the first byte of the pet: 02 black, 04 blue, 06 gold (section 7.2).
constexpr std::uint8_t kFenrirGroup = 13;
constexpr std::uint8_t kFenrirNumber = 37;
constexpr std::uint8_t kFenrirBits = 0x0E;

[[nodiscard]] std::uint8_t ByteAt(std::span<const std::byte> bytes, std::size_t index) noexcept {
    return index < bytes.size() ? std::to_integer<std::uint8_t>(bytes[index]) : std::uint8_t{0};
}

// The first two bytes of an item: the group in the high nibble of the first, and the number in 12 bits, with
// its bits 11 to 8 in the low nibble of the first. The number of the wings also shifts by 8. An empty slot is
// FF FF.
[[nodiscard]] characters::WornItem ReadItem(std::uint8_t first, std::uint8_t second) noexcept {
    characters::WornItem item;
    item.present = !(first == 0xFF && second == 0xFF);
    if (item.present) {
        item.group = static_cast<std::uint8_t>(first >> 4U);
        item.number = static_cast<std::uint16_t>(((first & 0x0FU) << 8U) | second);
    }
    return item;
}

[[nodiscard]] characters::WornItem ReadItemWithGlow(std::span<const std::byte> bytes,
                                                    std::size_t at) noexcept {
    characters::WornItem item = ReadItem(ByteAt(bytes, at), ByteAt(bytes, at + 1));
    if (item.present) {
        const std::uint8_t marks = ByteAt(bytes, at + 2);
        item.glow = static_cast<std::uint8_t>(marks >> 4U);
        item.excellent = (marks & kExcellent) != 0;
        item.ancient = (marks & kAncient) != 0;
    }
    return item;
}

void ReadPet(std::span<const std::byte> bytes, characters::Appearance& appearance) noexcept {
    const std::uint8_t first = ByteAt(bytes, kPetAt);
    const std::uint8_t second = ByteAt(bytes, kPetAt + 1);
    appearance.pet = ReadItem(first, second);
    if (appearance.pet.present && appearance.pet.group == kFenrirGroup && second == kFenrirNumber) {
        appearance.pet.number = kFenrirNumber;
        appearance.fenrir = static_cast<characters::FenrirVariant>((first & kFenrirBits) >> 1U);
    }
}

} // namespace

characters::Appearance ReadExtendedAppearance(std::span<const std::byte> bytes) noexcept {
    characters::Appearance appearance;
    appearance.characterClass = ByteAt(bytes, 0);
    const std::uint8_t marks = ByteAt(bytes, 1);
    appearance.pose = static_cast<std::uint8_t>(marks & kPoseMask);
    appearance.fullAncientSet = (marks & kFullAncientSet) != 0;
    appearance.gameMaster = (marks & kGameMaster) != 0;
    for (std::size_t slot = 0; slot < appearance.worn.size(); ++slot) {
        appearance.worn[slot] = ReadItemWithGlow(bytes, kFirstWorn + (3 * slot));
    }
    appearance.wings = ReadItem(ByteAt(bytes, kWingsAt), ByteAt(bytes, kWingsAt + 1));
    ReadPet(bytes, appearance);
    return appearance;
}

} // namespace mu::protocol
