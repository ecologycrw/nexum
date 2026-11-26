// The extended item (CA-04), with the vectors of the finding 2026-11-21-item-estendido.md of the knowledge
// center (section "Vetores de teste") and the gloves of a real session (finding
// 2026-11-21-capturas-reais-sessao-106-3.md, section 3).
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/ExtendedItem.hpp"

namespace {

using mu::items::kEmptySocket;
using mu::protocol::ReadExtendedItem;

[[nodiscard]] std::vector<std::byte> Bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

struct Combination {
    std::uint8_t flags;
    std::size_t size;
    std::vector<std::byte> bytes;
};

// The 32 combinations of the flags that add bytes, all with group 0, number 22, level 9 and durability 40:
// the option is type 1 level 3 (13), the excellent byte 3F, the ancient set 1 with bonus 2 (21), the harmony
// number 3 level 5 (35), and the sockets have no bonus and 3 sockets: empty, 00 and 3E (F3 FE 00 3E).
[[nodiscard]] std::vector<Combination> Combinations() {
    return {
        {0x00, 5, Bytes({0x00, 0x16, 0x09, 0x28, 0x00})},
        {0x01, 6, Bytes({0x00, 0x16, 0x09, 0x28, 0x01, 0x13})},
        {0x08, 6, Bytes({0x00, 0x16, 0x09, 0x28, 0x08, 0x3F})},
        {0x09, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x09, 0x13, 0x3F})},
        {0x10, 6, Bytes({0x00, 0x16, 0x09, 0x28, 0x10, 0x21})},
        {0x11, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x11, 0x13, 0x21})},
        {0x18, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x18, 0x3F, 0x21})},
        {0x19, 8, Bytes({0x00, 0x16, 0x09, 0x28, 0x19, 0x13, 0x3F, 0x21})},
        {0x20, 6, Bytes({0x00, 0x16, 0x09, 0x28, 0x20, 0x35})},
        {0x21, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x21, 0x13, 0x35})},
        {0x28, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x28, 0x3F, 0x35})},
        {0x29, 8, Bytes({0x00, 0x16, 0x09, 0x28, 0x29, 0x13, 0x3F, 0x35})},
        {0x30, 7, Bytes({0x00, 0x16, 0x09, 0x28, 0x30, 0x21, 0x35})},
        {0x31, 8, Bytes({0x00, 0x16, 0x09, 0x28, 0x31, 0x13, 0x21, 0x35})},
        {0x38, 8, Bytes({0x00, 0x16, 0x09, 0x28, 0x38, 0x3F, 0x21, 0x35})},
        {0x39, 9, Bytes({0x00, 0x16, 0x09, 0x28, 0x39, 0x13, 0x3F, 0x21, 0x35})},
        {0x80, 9, Bytes({0x00, 0x16, 0x09, 0x28, 0x80, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x81, 10, Bytes({0x00, 0x16, 0x09, 0x28, 0x81, 0x13, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x88, 10, Bytes({0x00, 0x16, 0x09, 0x28, 0x88, 0x3F, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x89, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0x89, 0x13, 0x3F, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x90, 10, Bytes({0x00, 0x16, 0x09, 0x28, 0x90, 0x21, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x91, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0x91, 0x13, 0x21, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x98, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0x98, 0x3F, 0x21, 0xF3, 0xFE, 0x00, 0x3E})},
        {0x99, 12, Bytes({0x00, 0x16, 0x09, 0x28, 0x99, 0x13, 0x3F, 0x21, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xA0, 10, Bytes({0x00, 0x16, 0x09, 0x28, 0xA0, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xA1, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0xA1, 0x13, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xA8, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0xA8, 0x3F, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xA9, 12, Bytes({0x00, 0x16, 0x09, 0x28, 0xA9, 0x13, 0x3F, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xB0, 11, Bytes({0x00, 0x16, 0x09, 0x28, 0xB0, 0x21, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xB1, 12, Bytes({0x00, 0x16, 0x09, 0x28, 0xB1, 0x13, 0x21, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xB8, 12, Bytes({0x00, 0x16, 0x09, 0x28, 0xB8, 0x3F, 0x21, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
        {0xB9, 13, Bytes({0x00, 0x16, 0x09, 0x28, 0xB9, 0x13, 0x3F, 0x21, 0x35, 0xF3, 0xFE, 0x00, 0x3E})},
    };
}

// The fields every combination must read, from the flags of byte 4.
void CheckCombination(const mu::protocol::ExtendedItem& read, std::uint8_t flags, std::size_t size) {
    const auto& item = read.item;
    CHECK(read.size == size);
    CHECK(item.group == 0);
    CHECK(item.number == 22);
    CHECK(item.level == 9);
    CHECK(item.durability == 40);
    CHECK(item.hasOption == ((flags & 0x01) != 0));
    CHECK(item.optionType == ((flags & 0x01) != 0 ? 1 : 0));
    CHECK(item.optionLevel == ((flags & 0x01) != 0 ? 3 : 0));
    CHECK(item.luck == ((flags & 0x02) != 0));
    CHECK(item.skill == ((flags & 0x04) != 0));
    CHECK(item.excellent == ((flags & 0x08) != 0 ? 0x3F : 0));
    CHECK(item.hasAncient == ((flags & 0x10) != 0));
    CHECK(item.ancientSet == ((flags & 0x10) != 0 ? 1 : 0));
    CHECK(item.ancientBonusLevel == ((flags & 0x10) != 0 ? 2 : 0));
    CHECK(item.hasHarmony == ((flags & 0x20) != 0));
    CHECK(item.harmonyNumber == ((flags & 0x20) != 0 ? 3 : 0));
    CHECK(item.harmonyLevel == ((flags & 0x20) != 0 ? 5 : 0));
    CHECK(item.option380 == ((flags & 0x40) != 0));
    CHECK(item.hasSockets == ((flags & 0x80) != 0));
    if ((flags & 0x80) != 0) {
        CHECK(item.socketBonus == 0x0F);
        CHECK(item.socketCount == 3);
        CHECK(item.sockets[0] == kEmptySocket);
        CHECK(item.sockets[1] == 0x00);
        CHECK(item.sockets[2] == 0x3E);
    }
}

} // namespace

TEST_CASE("[0001-CA-04] the 32 combinations of the flags that add bytes read with the size the flags give") {
    for (const auto& combination : Combinations()) {
        CAPTURE(combination.flags);
        CheckCombination(ReadExtendedItem(combination.bytes), combination.flags, combination.size);
    }
}

TEST_CASE(
    "[0001-CA-04] the other 224 combinations only change byte 4: luck, skill and option 380 add no byte") {
    int checked = 0;
    for (const auto& combination : Combinations()) {
        for (const std::uint8_t bitsWithoutBytes :
             std::array<std::uint8_t, 7>{0x02, 0x04, 0x06, 0x40, 0x42, 0x44, 0x46}) {
            auto bytes = combination.bytes;
            const auto flags = static_cast<std::uint8_t>(combination.flags | bitsWithoutBytes);
            bytes[4] = std::byte{flags};
            CAPTURE(flags);
            CheckCombination(ReadExtendedItem(bytes), flags, combination.size);
            ++checked;
        }
    }
    CHECK(checked == 224);
    // The two examples of the finding: 46 alone, and FF with every part.
    CHECK(ReadExtendedItem(Bytes({0x00, 0x16, 0x09, 0x28, 0x46})).size == 5);
    CheckCombination(ReadExtendedItem(Bytes(
                         {0x00, 0x16, 0x09, 0x28, 0xFF, 0x13, 0x3F, 0x21, 0x35, 0xF3, 0xFE, 0x00, 0x3E})),
                     0xFF, 13);
}

TEST_CASE("[0001-CA-04] several items one after the other stay aligned by the size of each one") {
    // An inventory reads item after item; one byte off would shift every item after it.
    std::vector<std::byte> bytes;
    for (const auto& combination : Combinations()) {
        bytes.insert(bytes.end(), combination.bytes.begin(), combination.bytes.end());
    }
    std::span<const std::byte> rest(bytes);
    for (const auto& combination : Combinations()) {
        const auto read = ReadExtendedItem(rest);
        CheckCombination(read, combination.flags, combination.size);
        rest = rest.subspan(read.size);
    }
    CHECK(rest.empty());
}

TEST_CASE("[0001-CA-04] the edge cases of the finding read as the server cut them") {
    const auto bigGroup = ReadExtendedItem(Bytes({0x10, 0x16, 0x00, 0x00, 0x00}));
    CHECK(bigGroup.item.group == 1);
    CHECK(bigGroup.item.number == 22);
    CHECK(ReadExtendedItem(Bytes({0x00, 0x01, 0x2C, 0x00, 0x00})).item.level == 44);
    CHECK(ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0xFF, 0x00})).item.durability == 255);
    CHECK(ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x0D, 0x00})).item.durability == 13);

    const auto option = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x01, 0x10}));
    CHECK(option.item.optionType == 1);
    CHECK(option.item.optionLevel == 0);

    const auto harmony = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x20, 0x30}));
    CHECK(harmony.item.harmonyNumber == 3);
    CHECK(harmony.item.harmonyLevel == 0);

    const auto twoEmpty = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x80, 0xF2, 0xFE, 0xFE}));
    CHECK(twoEmpty.size == 8);
    CHECK(twoEmpty.item.socketBonus == 0x0F);
    CHECK(twoEmpty.item.socketCount == 2);
    CHECK(twoEmpty.item.sockets[0] == kEmptySocket);
    CHECK(twoEmpty.item.sockets[1] == kEmptySocket);

    const auto bonus = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x80, 0x31, 0xFE}));
    CHECK(bonus.size == 7);
    CHECK(bonus.item.socketBonus == 3);
    CHECK(bonus.item.socketCount == 1);
}

TEST_CASE("[0001-CA-04] the socket bytes keep the value the server wrote") {
    // Sphere level x 50 + the offset of the element + the option: 00, 0C (1, 1, 2), 75 (3, 2, 1), EC (5, 5,
    // 0).
    const auto item =
        ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x80, 0xF5, 0x00, 0x0C, 0x75, 0xEC, 0xC8}));
    CHECK(item.size == 11);
    CHECK(item.item.socketCount == 5);
    CHECK(item.item.sockets == std::array<std::uint8_t, 15>{0x00, 0x0C, 0x75, 0xEC, 0xC8});
}

TEST_CASE(
    "[0001-CA-04] the gloves of a real session: group 10, number 5, durability 30 and one excellent option") {
    const auto gloves = ReadExtendedItem(Bytes({0xA0, 0x05, 0x00, 0x1E, 0x08, 0x04}));
    CHECK(gloves.size == 6);
    CHECK(gloves.item.group == 10);
    CHECK(gloves.item.number == 5);
    CHECK(gloves.item.level == 0);
    CHECK(gloves.item.durability == 30);
    CHECK(gloves.item.excellent == 0x04);
    CHECK_FALSE(gloves.item.hasOption);
}

TEST_CASE("[0001-CA-04] a cut item reads the missing bytes as 0 and never reads past the end") {
    // The flags ask for an option and an excellent byte, and the excellent byte is missing.
    const auto cut = ReadExtendedItem(Bytes({0x00, 0x16, 0x09, 0x28, 0x09, 0x13}));
    CHECK(cut.size == 6);
    CHECK(cut.item.optionType == 1);
    CHECK(cut.item.excellent == 0);

    const auto twoBytes = ReadExtendedItem(Bytes({0x00, 0x16}));
    CHECK(twoBytes.size == 2);
    CHECK(twoBytes.item.number == 22);
    CHECK(twoBytes.item.level == 0);

    // The sockets flag without its count byte: no socket, and the size stops at the end.
    const auto noCount = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x80}));
    CHECK(noCount.size == 5);
    CHECK(noCount.item.socketCount == 0);

    // 15 sockets declared and only 2 there: the size is what is there, and the missing sockets read as 0.
    const auto fewSockets = ReadExtendedItem(Bytes({0x00, 0x01, 0x00, 0x00, 0x80, 0xFF, 0xFE, 0xFE}));
    CHECK(fewSockets.size == 8);
    CHECK(fewSockets.item.socketCount == 15);
    CHECK(fewSockets.item.sockets[1] == kEmptySocket);
    CHECK(fewSockets.item.sockets[2] == 0);

    CHECK(ReadExtendedItem(std::span<const std::byte>{}).size == 0);
}
