// The translation of the packets of the world into neutral events (CA-09). The examples come from the plan of
// slice 1 ("Comportamento: os pacotes do mundo e os eventos"): the C2 12 and the echo of the walk of a real
// session (finding 2026-11-21-capturas-reais-sessao-106-3.md, section 3, and the V2 of the finding
// 2026-11-22-direcoes-do-walkrequest.md), and the C2 13 and the C1 14 calculated from their layout (finding
// 2026-11-21-variantes-por-versao-m2.md, section 4). The other packets are calculated from the layout of the
// protocol spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.11, 6.12, 7
// and 8) and of the XML of OpenMU.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/Dialect/FirstEntry.hpp"
#include "Mu/Protocol/PacketFields.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

namespace {

namespace dialect = mu::protocol::dialect;
namespace world = mu::world;
using Bytes = std::vector<std::byte>;
using nexum::debug::LogLevel;

constexpr world::ObjectId kHero{0x0200};

[[nodiscard]] Bytes Make(std::initializer_list<std::uint8_t> values) {
    Bytes bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

[[nodiscard]] Bytes Join(std::initializer_list<Bytes> parts) {
    Bytes joined;
    for (const auto& part : parts) {
        joined.insert(joined.end(), part.begin(), part.end());
    }
    return joined;
}

void PutLittleEndian(Bytes& bytes, std::size_t offset, std::uint32_t value, std::size_t size) {
    for (std::size_t index = 0; index < size; ++index) {
        bytes[offset + index] = std::byte{static_cast<std::uint8_t>(value >> (8U * index))};
    }
}

// A DarkWizard standing, with nothing on (finding 2026-11-21-aparencia-estendida.md, section 4).
[[nodiscard]] Bytes NothingOn() {
    return Make({0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
                 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF});
}

// The C2 12 of the player himself in a real session: the 26 bytes before the appearance come from the
// capture, then the appearance of a DarkWizard with nothing on and the visible effects. The size is 54 bytes
// plus 1 for each effect (finding 2026-11-21-aparencia-estendida.md, section 2).
[[nodiscard]] Bytes PlayerHimself(std::initializer_list<std::uint8_t> effects = {}) {
    Bytes player = Join({Make({0xC2, 0x00, 0x36, 0x12, 0x00, 0x02, 0x91, 0x7B, 0x91, 0x7B, 0x00, 0x00, 0x1D,
                               0x00, 0x1D, 0x00, 0x74, 0x65, 0x73, 0x74, 0x30, 0x44, 0x6B, 0x00, 0x00, 0x00}),
                         NothingOn(), Make({static_cast<std::uint8_t>(effects.size())}), Make(effects)});
    player[2] = std::byte{static_cast<std::uint8_t>(player.size())};
    return player;
}

// The data of the character, at (145, 123) on the map 0, with its values at the offsets of the XML of OpenMU
// (CharacterInformationExtended): the health at 36, the mana at 44, the shield at 52 and the ability at 60,
// each followed by its maximum, and the speeds at 82 and 84, all little-endian.
[[nodiscard]] Bytes CharacterInformation(std::size_t size = 93) {
    Bytes data(size);
    data[0] = std::byte{0xC3};
    data[1] = std::byte{static_cast<std::uint8_t>(size)};
    data[2] = std::byte{0xF3};
    data[3] = std::byte{0x03};
    data[4] = std::byte{145};
    data[5] = std::byte{123};
    PutLittleEndian(data, 36, 110, 4);
    PutLittleEndian(data, 40, 120, 4);
    PutLittleEndian(data, 44, 20, 4);
    PutLittleEndian(data, 48, 30, 4);
    PutLittleEndian(data, 52, 5, 4);
    PutLittleEndian(data, 56, 6, 4);
    PutLittleEndian(data, 60, 15, 4);
    PutLittleEndian(data, 64, 16, 4);
    PutLittleEndian(data, 82, 29, 2);
    PutLittleEndian(data, 84, 29, 2);
    return data;
}

// A packet of this shape with zeros in every field, as small as the shape allows.
[[nodiscard]] Bytes Empty(const mu::protocol::PacketShape& shape) {
    Bytes bytes(shape.minimumLength);
    REQUIRE(mu::protocol::StartPacket(bytes, shape).has_value());
    return bytes;
}

struct Translation {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink, LogLevel::Debug};
    mu::translate::Translator translator{kHero, log};
    world::EventQueue queue{64};

    // The events of these messages, in order.
    std::vector<world::WorldEvent> Events(std::initializer_list<Bytes> messages) {
        for (const Bytes& message : messages) {
            translator.Translate(message, queue);
        }
        std::vector<world::WorldEvent> events;
        queue.Drain([&](const world::WorldEvent& event) { events.push_back(event); });
        return events;
    }

    [[nodiscard]] std::size_t LogCount(std::string_view text) const {
        return static_cast<std::size_t>(std::ranges::count_if(sink.Entries(), [&](const auto& entry) {
            return entry.message.find(text) != std::string::npos;
        }));
    }
};

} // namespace

TEST_CASE("[0001-CA-09] the C2 12 of the hero of a real session becomes a player that came into view") {
    Translation translation;
    const auto events = translation.Events({PlayerHimself()});
    REQUIRE(events.size() == 1);
    const auto& entered = std::get<world::ObjectEntered>(events[0]);
    CHECK(entered.id == kHero);
    CHECK(entered.kind == world::ObjectKind::Player);
    CHECK_FALSE(entered.justAppeared);
    CHECK(entered.position == world::Cell{145, 123});
    CHECK(entered.target == world::Cell{145, 123});
    CHECK(entered.rotation == 0);
    CHECK(entered.name.View() == "test0Dk");
    CHECK(entered.appearance.characterClass == 0);
    CHECK_FALSE(entered.appearance.Worn(mu::characters::WornSlot::Armor).present);
    CHECK(entered.effectCount == 0);
}

TEST_CASE("[0001-CA-09] the visible effects of a player are counted, and a player shorter than 54 bytes is "
          "dropped") {
    Translation translation;
    const auto withEffects = translation.Events({PlayerHimself({0x01, 0x07})});
    REQUIRE(withEffects.size() == 1);
    CHECK(std::get<world::ObjectEntered>(withEffects[0]).effectCount == 2);

    Bytes cut = PlayerHimself();
    cut.pop_back();
    cut[2] = std::byte{53};
    CHECK(translation.Events({cut}).empty());
    CHECK(
        translation.LogCount("dropped the packet AddCharacterToScopeExtended: it has 53 bytes, and a player "
                             "needs at least 54") == 1);

    Bytes fewEffects = PlayerHimself({0x01, 0x07});
    fewEffects.pop_back();
    fewEffects[2] = std::byte{55};
    CHECK(translation.Events({fewEffects}).empty());
    CHECK(translation.LogCount("it declares 2 effects, but holds only 1") == 1);
}

TEST_CASE("[0001-CA-09] the echo of the walk of a real session: the object, the source, the target and the "
          "rotation before the walk") {
    Translation translation;
    const auto events =
        translation.Events({Make({0xC1, 0x0A, 0xD4, 0x02, 0x00, 0x91, 0x7B, 0x90, 0x7E, 0x30})});
    REQUIRE(events.size() == 1);
    const auto& walking = std::get<world::ObjectStartedWalking>(events[0]);
    CHECK(walking.id == kHero);
    CHECK(walking.source == world::Cell{145, 123});
    CHECK(walking.target == world::Cell{144, 126});
    CHECK(walking.rotation == 3);
    CHECK(walking.stepCount == 0);
}

TEST_CASE("[0001-CA-09] an NPC that just appeared: the mark of bit 15 is apart from the id") {
    Translation translation;
    const auto events = translation.Events(
        {Make({0xC2, 0x00, 0x0F, 0x13, 0x01, 0x80, 0x21, 0x00, 0x0E, 0x87, 0x7C, 0x87, 0x7C, 0x30, 0x00})});
    REQUIRE(events.size() == 1);
    const auto& entered = std::get<world::ObjectEntered>(events[0]);
    CHECK(entered.id == world::ObjectId{0x0021});
    CHECK(entered.kind == world::ObjectKind::NpcOrMonster);
    CHECK(entered.justAppeared);
    CHECK(entered.typeNumber == 14);
    CHECK(entered.position == world::Cell{135, 124});
    CHECK(entered.target == world::Cell{135, 124});
    CHECK(entered.rotation == 3);
}

TEST_CASE("[0001-CA-09] each NPC of a C2 13 and each id of a C1 14 is an event of its own, in order") {
    Translation translation;
    const auto events = translation.Events(
        {Make({0xC2, 0x00, 0x19, 0x13, 0x02, 0x00, 0x21, 0x00, 0x0E, 0x87, 0x7C, 0x88, 0x7C,
               0x30, 0x00, 0x00, 0x22, 0x01, 0x02, 0x10, 0x20, 0x10, 0x20, 0x10, 0x00}),
         Make({0xC1, 0x08, 0x14, 0x02, 0x00, 0x21, 0x00, 0x22})});
    REQUIRE(events.size() == 4);
    const auto& first = std::get<world::ObjectEntered>(events[0]);
    CHECK(first.id == world::ObjectId{0x0021});
    CHECK_FALSE(first.justAppeared);
    CHECK(first.target == world::Cell{136, 124});
    const auto& second = std::get<world::ObjectEntered>(events[1]);
    CHECK(second.id == world::ObjectId{0x0022});
    CHECK(second.typeNumber == 0x0102);
    CHECK(second.position == world::Cell{16, 32});
    CHECK(second.rotation == 1);
    CHECK(std::get<world::ObjectLeft>(events[2]).id == world::ObjectId{0x0021});
    CHECK(std::get<world::ObjectLeft>(events[3]).id == world::ObjectId{0x0022});
}

TEST_CASE("[0001-CA-09] the C1 14 of the plan: the object 0x0021 left the view") {
    Translation translation;
    const auto events = translation.Events({Make({0xC1, 0x06, 0x14, 0x01, 0x00, 0x21})});
    REQUIRE(events.size() == 1);
    CHECK(std::get<world::ObjectLeft>(events[0]).id == world::ObjectId{0x0021});
}

TEST_CASE("[0001-CA-09] a list that declares more entries than it holds is dropped whole") {
    Translation translation;
    CHECK(translation
              .Events({Make({0xC2, 0x00, 0x0F, 0x13, 0x02, 0x00, 0x21, 0x00, 0x0E, 0x87, 0x7C, 0x87, 0x7C,
                             0x30, 0x00}),
                       Make({0xC1, 0x06, 0x14, 0x02, 0x00, 0x21}),
                       Make({0xC1, 0x0B, 0x46, 0x00, 0x01, 0x00, 0x02, 0x0A, 0x14, 0x1E, 0x28})})
              .empty());
    CHECK(translation.LogCount("dropped the packet AddNpcsToScope: it declares 2 NPCs or monsters, but holds "
                               "only 1") == 1);
    CHECK(
        translation.LogCount("dropped the packet MapObjectOutOfScope: it declares 2 objects, but holds only "
                             "1") == 1);
    CHECK(translation.LogCount("dropped the packet ChangeTerrainAttributes: it declares 2 areas, but holds "
                               "only 1") == 1);
}

TEST_CASE("[0001-CA-09] the correction of the position (C1 15) puts the object in the cell of the server") {
    Translation translation;
    const auto events = translation.Events({Make({0xC1, 0x08, 0x15, 0x02, 0x00, 0x90, 0x7E, 0x00})});
    REQUIRE(events.size() == 1);
    const auto& corrected = std::get<world::PositionCorrected>(events[0]);
    CHECK(corrected.id == kHero);
    CHECK(corrected.position == world::Cell{144, 126});
}

TEST_CASE("[0001-CA-09] the stats of the hero (C1 26 FF): health, shield, mana, ability and the speeds") {
    Translation translation;
    const auto events =
        translation.Events({Make({0xC1, 0x18, 0x26, 0xFF, 0x6E, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
                                  0x14, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x1E, 0x00})});
    REQUIRE(events.size() == 1);
    const auto& stats = std::get<world::HeroStatsChanged>(events[0]).stats;
    CHECK(stats ==
          world::HeroStats{
              .health = 110, .shield = 5, .mana = 20, .ability = 15, .attackSpeed = 29, .magicSpeed = 30});
}

TEST_CASE("[0001-CA-09] the data of the character becomes the hero on its map, also with 92 bytes") {
    for (const std::size_t size : {std::size_t{93}, std::size_t{92}}) {
        Translation translation;
        const auto events = translation.Events({CharacterInformation(size)});
        REQUIRE(events.size() == 1);
        const auto& hero = std::get<world::HeroLoaded>(events[0]);
        CHECK(hero.id == kHero);
        CHECK(hero.map == 0);
        CHECK(hero.position == world::Cell{145, 123});
        CHECK(
            hero.stats ==
            world::HeroStats{
                .health = 110, .shield = 5, .mana = 20, .ability = 15, .attackSpeed = 29, .magicSpeed = 29});
        CHECK(hero.maxima == world::HeroMaxima{.health = 120, .shield = 6, .mana = 30, .ability = 16});
    }
    Translation shortData;
    CHECK(shortData.Events({CharacterInformation(91)}).empty());
    CHECK(shortData.LogCount("the data of the character has 91 bytes; it needs at least 92") == 1);
}

TEST_CASE("[0001-CA-09] the inventory is read: its size, then each slot and its item") {
    Translation translation;
    // Two entries: the slot 0 with the item 00 16 09 28 00 of the finding of the item, and the slot 5 with
    // the gloves A0 05 00 1E 08 04 of a real session (5 fixed bytes and 1 for the excellent option).
    const Bytes inventory = Make({0xC4, 0x00, 0x13, 0xF3, 0x10, 0x02, 0x00, 0x00, 0x16, 0x09, 0x28, 0x00,
                                  0x05, 0xA0, 0x05, 0x00, 0x1E, 0x08, 0x04});
    const auto events = translation.Events({inventory});
    REQUIRE(events.size() == 3);
    CHECK(std::get<world::InventoryReplaced>(events[0]).itemCount == 2);
    const auto& sword = std::get<world::InventoryItemPut>(events[1]);
    CHECK(sword.slot == 0);
    CHECK(sword.item.group == 0);
    CHECK(sword.item.number == 22);
    CHECK(sword.item.level == 9);
    CHECK(sword.item.durability == 40);
    const auto& gloves = std::get<world::InventoryItemPut>(events[2]);
    CHECK(gloves.slot == 5);
    CHECK(gloves.item.group == 10);
    CHECK(gloves.item.number == 5);
    CHECK(gloves.item.durability == 30);
    CHECK(gloves.item.excellent == 0x04);

    // The same inventory, declaring a third entry it does not hold.
    Bytes cut = inventory;
    cut[5] = std::byte{3};
    CHECK(translation.Events({cut}).empty());
    CHECK(translation.LogCount(
              "dropped the packet CharacterInventory: it declares 3 items, but holds only 2") == 1);
}

TEST_CASE("[0001-CA-09] the change of the terrain (C1 46) is read, one event for each area") {
    Translation translation;
    // Add the attribute 01 (Safezone) from (10, 20) to (30, 40), then remove it from (1, 2) to (3, 4).
    const auto events =
        translation.Events({Make({0xC1, 0x0B, 0x46, 0x00, 0x01, 0x00, 0x01, 0x0A, 0x14, 0x1E, 0x28}),
                            Make({0xC1, 0x0B, 0x46, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04})});
    REQUIRE(events.size() == 2);
    const auto& added = std::get<world::TerrainChanged>(events[0]);
    CHECK(added.start == world::Cell{10, 20});
    CHECK(added.end == world::Cell{30, 40});
    CHECK(added.attributes == 0x01);
    CHECK_FALSE(added.removed);
    const auto& removed = std::get<world::TerrainChanged>(events[1]);
    CHECK(removed.start == world::Cell{1, 2});
    CHECK(removed.removed);
}

TEST_CASE("[0001-CA-09] the events come out in the order the packets came") {
    Translation translation;
    const auto events = translation.Events(
        {CharacterInformation(), PlayerHimself(), Make({0xC1, 0x06, 0x14, 0x01, 0x00, 0x21}),
         Make({0xC1, 0x0A, 0xD4, 0x02, 0x00, 0x91, 0x7B, 0x90, 0x7E, 0x30})});
    REQUIRE(events.size() == 4);
    CHECK(std::holds_alternative<world::HeroLoaded>(events[0]));
    CHECK(std::holds_alternative<world::ObjectEntered>(events[1]));
    CHECK(std::holds_alternative<world::ObjectLeft>(events[2]));
    CHECK(std::holds_alternative<world::ObjectStartedWalking>(events[3]));
}

TEST_CASE(
    "[0001-CA-09] an unknown code is ignored and logged once, with its subcode when the code has them") {
    Translation translation;
    CHECK(translation
              .Events({Make({0xC1, 0x05, 0x30, 0x01, 0x02}), Make({0xC1, 0x05, 0x30, 0x07, 0x08}),
                       Make({0xC1, 0x05, 0xF3, 0x99, 0x00}), Make({0xC1, 0x05, 0xF3, 0x99, 0x01}),
                       Make({0xC1, 0x05, 0xF3, 0x98, 0x00}), Make({0xC2, 0x00, 0x05, 0x31, 0x00})})
              .empty());
    CHECK(translation.LogCount("ignored a packet the client does not know") == 4);
    CHECK(translation.LogCount("does not know, C1 30 of 5 bytes") == 1);
    CHECK(translation.LogCount("does not know, C1 F3 99 of 5 bytes") == 1);
    CHECK(translation.LogCount("does not know, C1 F3 98 of 5 bytes") == 1);
    CHECK(translation.LogCount("does not know, C2 31 of 5 bytes") == 1);
}

TEST_CASE("[0001-CA-09] the known packets that the slice does not use are ignored without a word") {
    Translation translation;
    // The packets of the first entry (section 6.12 of the protocol spec) and the C1 B2 1A of the real
    // sessions.
    for (const auto* shape : {&dialect::MasterStatsUpdate::kShape,
                              &dialect::SkillListUpdate::kShape,
                              &dialect::KeyConfiguration::kShape,
                              &dialect::LegacyQuestStates::kShape,
                              &dialect::QuestStates::kShape,
                              &dialect::CashShopScriptVersion::kShape,
                              &dialect::CashShopBannerVersion::kShape,
                              &dialect::WarpList::kShape,
                              &dialect::ServerMessage::kShape,
                              &dialect::AddTransformedCharacterToScope::kShape,
                              &dialect::AddSummonedMonstersToScope::kShape,
                              &dialect::ItemsDropped::kShape,
                              &dialect::Weather::kShape,
                              &dialect::HelperConfiguration::kShape,
                              &dialect::GameMasterEffect::kShape,
                              &dialect::GuildAssignment::kShape,
                              &dialect::MessengerInitialization::kShape,
                              &dialect::FriendRequest::kShape,
                              &dialect::Letter::kShape,
                              &dialect::CastleSiegeTaxRate::kShape}) {
        CAPTURE(shape->name);
        CHECK(translation.Events({Empty(*shape)}).empty());
    }
    CHECK(translation.sink.Entries().empty());
}

TEST_CASE("[0001-CA-09] a queue of events that passes its room grows, with one warning, and loses nothing") {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    mu::translate::Translator translator{kHero, log};
    world::EventQueue queue{1};
    const Bytes leaving = Make({0xC1, 0x08, 0x14, 0x02, 0x00, 0x21, 0x00, 0x22});
    CHECK(translator.Translate(leaving, queue) == 2);
    CHECK(translator.Translate(leaving, queue) == 2);
    CHECK(queue.Count() == 4);
    REQUIRE(sink.Entries().size() == 1);
    CHECK(sink.Entries()[0].level == LogLevel::Warning);
    CHECK(sink.Entries()[0].message.find("passed the reserved room") != std::string::npos);
}
