// The extended appearance (CA-04), with the examples of the finding 2026-11-21-aparencia-estendida.md of the
// knowledge center (section 4) and the protocol spec, sections 7.2 and 11.
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/ExtendedAppearance.hpp"

namespace {

using mu::characters::FenrirVariant;
using mu::characters::WornSlot;
using mu::protocol::ReadExtendedAppearance;

[[nodiscard]] std::vector<std::byte> Bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

// A DarkWizard standing, with nothing on: 00 00, then 7 x FF FF 00, then FF FF FF FF.
[[nodiscard]] std::vector<std::byte> NothingOn() {
    return Bytes({0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
                  0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF});
}

} // namespace

TEST_CASE("[0001-CA-04] a DarkWizard standing with nothing on has no item in any slot") {
    const auto bytes = NothingOn();
    REQUIRE(bytes.size() == mu::protocol::kExtendedAppearanceSize);
    const auto appearance = ReadExtendedAppearance(bytes);
    CHECK(appearance.characterClass == 0);
    CHECK(appearance.pose == 0);
    CHECK_FALSE(appearance.fullAncientSet);
    CHECK_FALSE(appearance.gameMaster);
    for (const auto& worn : appearance.worn) {
        CHECK_FALSE(worn.present);
    }
    CHECK_FALSE(appearance.wings.present);
    CHECK_FALSE(appearance.pet.present);
    CHECK(appearance.fenrir == FenrirVariant::None);
}

TEST_CASE(
    "[0001-CA-04] the marks byte 30 is a game master with the full ancient set; the pose is in bits 0 to 2") {
    auto bytes = NothingOn();
    bytes[1] = std::byte{0x30};
    const auto marked = ReadExtendedAppearance(bytes);
    CHECK(marked.gameMaster);
    CHECK(marked.fullAncientSet);
    CHECK(marked.pose == 0);

    bytes[1] = std::byte{0x22};
    const auto sitting = ReadExtendedAppearance(bytes);
    CHECK(sitting.gameMaster);
    CHECK_FALSE(sitting.fullAncientSet);
    CHECK(sitting.pose == 2);
}

TEST_CASE("[0001-CA-04] an item with glow 00 01 48 is group 0, number 1, glow 4 and excellent") {
    auto bytes = NothingOn();
    bytes[5] = std::byte{0x00}; // the right hand
    bytes[6] = std::byte{0x01};
    bytes[7] = std::byte{0x48};
    const auto appearance = ReadExtendedAppearance(bytes);
    const auto& sword = appearance.Worn(WornSlot::RightHand);
    CHECK(sword.present);
    CHECK(sword.group == 0);
    CHECK(sword.number == 1);
    CHECK(sword.glow == 4); // (9 - 1) / 2: an item of level 9 or 10
    CHECK(sword.excellent);
    CHECK_FALSE(sword.ancient);
    CHECK_FALSE(appearance.Worn(WornSlot::LeftHand).present);
}

TEST_CASE("[0001-CA-04] the gloves of a real session keep the excellent mark of their third byte") {
    auto bytes = NothingOn();
    bytes[17] = std::byte{0xA0}; // the gloves, group 10 and number 5
    bytes[18] = std::byte{0x05};
    bytes[19] = std::byte{0x08};
    const auto appearance = ReadExtendedAppearance(bytes);
    const auto& gloves = appearance.Worn(WornSlot::Gloves);
    CHECK(gloves.group == 10);
    CHECK(gloves.number == 5);
    CHECK(gloves.excellent);
}

TEST_CASE("[0001-CA-04] the number of the wings uses the shift of 8 bits") {
    auto bytes = NothingOn();
    bytes[23] = std::byte{0xC0};
    bytes[24] = std::byte{0x05};
    const auto wings = ReadExtendedAppearance(bytes).wings;
    CHECK(wings.present);
    CHECK(wings.group == 12);
    CHECK(wings.number == 5);

    // A number from 256 on shows the shift: the low nibble of the first byte is bits 11 to 8, not 7 to 4.
    bytes[23] = std::byte{0xC1};
    CHECK(ReadExtendedAppearance(bytes).wings.number == 0x105);
}

TEST_CASE("[0001-CA-04] a golden Fenrir is the pet D6 25: group 13, number 37, with the variant bits 06") {
    auto bytes = NothingOn();
    bytes[25] = std::byte{0xD6};
    bytes[26] = std::byte{0x25};
    const auto appearance = ReadExtendedAppearance(bytes);
    CHECK(appearance.pet.group == 13);
    CHECK(appearance.pet.number == 37);
    CHECK(appearance.fenrir == FenrirVariant::Gold);

    bytes[25] = std::byte{0xD2};
    CHECK(ReadExtendedAppearance(bytes).fenrir == FenrirVariant::Black);
    bytes[25] = std::byte{0xD4};
    CHECK(ReadExtendedAppearance(bytes).fenrir == FenrirVariant::Blue);
    bytes[25] = std::byte{0xD0};
    CHECK(ReadExtendedAppearance(bytes).fenrir == FenrirVariant::None);
}

TEST_CASE("[0001-CA-04] cut appearance bytes read the missing bytes as 0 and never read past the end") {
    const auto bytes = NothingOn();
    const auto cut = ReadExtendedAppearance(std::span<const std::byte>(bytes).first(8));
    CHECK_FALSE(cut.Worn(WornSlot::LeftHand).present);
    CHECK_FALSE(cut.Worn(WornSlot::RightHand).present);
    // From the helm on, every byte is missing and reads as 0: an item of group 0 and number 0.
    CHECK(cut.Worn(WornSlot::Helm).present);
    CHECK(cut.Worn(WornSlot::Helm).number == 0);
    CHECK(cut.pet.present);

    const auto nothing = ReadExtendedAppearance(std::span<const std::byte>{});
    CHECK(nothing.characterClass == 0);
}
