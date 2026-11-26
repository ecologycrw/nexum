#pragma once

// An item as the game knows it, whatever the bytes it came in: the inventory, and later the shops and the
// vault. The meaning of each part comes from the protocol spec of the knowledge center
// (docs/specs/protocolo/protocolo-106-3-m2.md, section 7.1).

#include <array>
#include <cstdint>

namespace mu::items {

struct Item {
    std::uint8_t group = 0;   // 0 to 15
    std::uint16_t number = 0; // 0 to 4095
    std::uint8_t level = 0;
    std::uint8_t durability = 0;

    bool hasOption = false;
    std::uint8_t optionType = 0; // the fixed options travel as bits of the level, with the type 0
    std::uint8_t optionLevel = 0;
    bool luck = false;
    bool skill = false;
    std::uint8_t excellent = 0; // one bit per excellent option: the bit (number - 1)

    bool hasAncient = false;
    std::uint8_t ancientSet = 0;
    std::uint8_t ancientBonusLevel = 0;

    bool hasHarmony = false;
    std::uint8_t harmonyNumber = 0;
    std::uint8_t harmonyLevel = 0;

    bool option380 = false;

    bool hasSockets = false;
    std::uint8_t socketBonus = 0x0F; // 0x0F, and also 0, mean no bonus
    std::uint8_t socketCount = 0;    // the count the item declares, up to 15
    std::array<std::uint8_t, 15> sockets{};

    friend bool operator==(const Item&, const Item&) = default;
};

// A socket that has no seed sphere (section 7.1).
inline constexpr std::uint8_t kEmptySocket = 0xFE;

} // namespace mu::items
