#pragma once

// How a character looks to the others: the class, the pose, the marks, and a summary of each item it wears.
// It is the same for the list of characters and for the players that come into view. The meaning of each part
// comes from the protocol spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md,
// section 7.2).

#include <array>
#include <cstdint>

namespace mu::characters {

// A worn item as the others see it: no options, only what is needed to draw it.
struct WornItem {
    bool present = false;
    std::uint8_t group = 0;
    std::uint16_t number = 0;
    std::uint8_t glow = 0; // (item level - 1) / 2, from 0 to 7; only the items of the hands and of the armor
    bool excellent = false;
    bool ancient = false;

    friend bool operator==(const WornItem&, const WornItem&) = default;
};

// The places of the 7 items with glow, in the order of the bytes.
enum class WornSlot : std::uint8_t { LeftHand, RightHand, Helm, Armor, Pants, Gloves, Boots };

// The variants of the Fenrir pet, which travel as bits over the number of the pet.
enum class FenrirVariant : std::uint8_t { None, Black, Blue, Gold };

struct Appearance {
    std::uint8_t characterClass = 0; // 0 DarkWizard, 2 SoulMaster, 3 GrandMaster, 4 DarkKnight, ...
    std::uint8_t pose = 0;           // 0 standing, 2 sitting, 3 leaning, 4 hanging
    bool fullAncientSet = false;
    bool gameMaster = false;
    std::array<WornItem, 7> worn{};
    WornItem wings;
    WornItem pet;
    FenrirVariant fenrir = FenrirVariant::None;

    [[nodiscard]] const WornItem& Worn(WornSlot slot) const noexcept {
        return worn[static_cast<std::size_t>(slot)];
    }

    friend bool operator==(const Appearance&, const Appearance&) = default;
};

} // namespace mu::characters
