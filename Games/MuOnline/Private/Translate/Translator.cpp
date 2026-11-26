#include "Mu/Translate/Translator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "Mu/Protocol/CharacterInformation.hpp"
#include "Mu/Protocol/Dialect.hpp"
#include "Mu/Protocol/ExtendedAppearance.hpp"
#include "Mu/Protocol/ExtendedItem.hpp"
#include "Mu/Protocol/PacketFields.hpp"
#include "Nexum/Debug/LogLine.hpp"

namespace mu::translate {

namespace {

namespace dialect = protocol::dialect;
using nexum::debug::LogLevel;
using nexum::debug::LogLine;
using world::Cell;
using world::ObjectId;

constexpr nexum::debug::LogCategory kTranslate{"translate"};

// The packets this slice translates (CA-09), in the order of their translations in TranslateWorldPacket.
constexpr std::array kTranslated{
    &dialect::CharacterInformation::kShape,
    &dialect::CurrentStats::kShape,
    &dialect::AddCharacterToScope::kShape,
    &dialect::AddNpcsToScope::kShape,
    &dialect::MapObjectOutOfScope::kShape,
    &dialect::ObjectWalked::kShape,
    &dialect::ObjectMoved::kShape,
    &dialect::CharacterInventory::kShape,
    &dialect::ChangeTerrainAttributes::kShape,
};

// The packets that come with the first entry and that this slice does not use (section 6.12 of the protocol
// spec, and the C1 B2 1A of the real sessions): ignored without a warning. The transformed characters and the
// summons enter the view in M4 (TD-0030), and the items on the ground in M5 (TD-0031).
constexpr std::array kIgnored{
    &dialect::MasterStatsUpdate::kShape,
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
    &dialect::CastleSiegeTaxRate::kShape,
};

// Bit 15 of an id marks an object that just appeared (finding 2026-11-21-aparencia-estendida.md, section 2,
// and finding 2026-11-21-variantes-por-versao-m2.md, section 4). The other packets never set it, so every id
// is read without it.
constexpr std::uint16_t kJustAppeared = 0x8000;
constexpr std::uint16_t kIdBits = 0x7FFF;

[[nodiscard]] ObjectId IdOf(std::uint16_t raw) noexcept {
    return ObjectId{static_cast<std::uint16_t>(raw & kIdBits)};
}

[[nodiscard]] bool JustAppeared(std::uint16_t raw) noexcept {
    return (raw & kJustAppeared) != 0;
}

// A player is 54 bytes plus 1 for each visible effect: the appearance of 27 bytes starts at byte 26, and the
// count of effects is byte 53 (finding 2026-11-21-aparencia-estendida.md, section 2).
constexpr std::size_t kPlayerBytes = 54;

// Each entry of the inventory is the slot, then the item, whose 5 first bytes are always there (sections 6.12
// and 7.1 of the protocol spec).
constexpr std::size_t kSlotBytes = 1;
constexpr std::size_t kFixedItemBytes = 5;

[[nodiscard]] bool IsAny(std::span<const std::byte> message,
                         std::span<const protocol::PacketShape* const> shapes) {
    return std::ranges::any_of(
        shapes, [&](const protocol::PacketShape* shape) { return protocol::IsPacket(message, *shape); });
}

// Whether any packet of this slice with this code has a subcode, so that an unknown packet of the code is
// told apart by its subcode too.
[[nodiscard]] bool HasSubCodes(std::uint8_t code) noexcept {
    const auto hasSubCode = [code](const protocol::PacketShape* shape) {
        return shape->code == code && shape->hasSubCode;
    };
    return std::ranges::any_of(kTranslated, hasSubCode) || std::ranges::any_of(kIgnored, hasSubCode);
}

// The first entry of a list of `count` that `entryAt(index)` cannot read, or none when every entry is there.
template <typename EntryAt>
[[nodiscard]] std::optional<std::size_t> FirstMissing(std::size_t count, EntryAt&& entryAt) {
    for (std::size_t index = 0; index < count; ++index) {
        if (!entryAt(index)) {
            return index;
        }
    }
    return std::nullopt;
}

// "it declares 3 NPCs, but holds only 2".
void DescribeMissing(LogLine& reason, std::size_t declared, std::string_view what,
                     std::size_t held) noexcept {
    reason.Append("it declares ").AppendNumber(declared).Append(" ").Append(what);
    reason.Append(", but holds only ").AppendNumber(held);
}

// Gives each entry of the inventory to `visit(slot, item)`, in order. Gives how many entries were there
// whole, at least the slot and the 5 fixed bytes of the item; a byte of the item past the end reads as 0
// (section 7.1).
template <typename Visit>
std::size_t ForEachInventoryEntry(std::span<const std::byte> entries, std::size_t count, Visit&& visit) {
    std::size_t start = 0;
    for (std::size_t index = 0; index < count; ++index) {
        if (entries.size() - start < kSlotBytes + kFixedItemBytes) {
            return index;
        }
        const auto slot = protocol::ReadByte(entries, start);
        const auto item = protocol::ReadExtendedItem(entries.subspan(start + kSlotBytes));
        visit(slot, item.item);
        start += kSlotBytes + item.size;
    }
    return count;
}

} // namespace

Translator::Translator(ObjectId hero, nexum::debug::Logger& log) : m_hero(hero), m_log(&log) {}

std::size_t Translator::Translate(Message message, world::EventQueue& events) {
    const std::size_t before = events.Count();
    if (!TranslateWorldPacket(message, events) && !IsAny(message, kIgnored)) {
        ReportUnknown(message);
    }
    return events.Count() - before;
}

bool Translator::TranslateWorldPacket(Message message, world::EventQueue& events) {
    using Translation = void (Translator::*)(Message, world::EventQueue&);
    struct Route {
        const protocol::PacketShape* shape;
        Translation translate;
    };
    // The C1 26 FF, the C1 D4 and the C2 12 have other variants with the same code; the dialect says which
    // one the 106.3 receives.
    static constexpr std::array<Route, kTranslated.size()> kRoutes{{
        {&dialect::CharacterInformation::kShape, &Translator::TranslateHero},
        {&dialect::CurrentStats::kShape, &Translator::TranslateStats},
        {&dialect::AddCharacterToScope::kShape, &Translator::TranslatePlayer},
        {&dialect::AddNpcsToScope::kShape, &Translator::TranslateNpcs},
        {&dialect::MapObjectOutOfScope::kShape, &Translator::TranslateLeaving},
        {&dialect::ObjectWalked::kShape, &Translator::TranslateWalk},
        {&dialect::ObjectMoved::kShape, &Translator::TranslateCorrection},
        {&dialect::CharacterInventory::kShape, &Translator::TranslateInventory},
        {&dialect::ChangeTerrainAttributes::kShape, &Translator::TranslateTerrain},
    }};
    // Every packet of kTranslated, which tells the codes with subcodes, has its translation here, in the same
    // order.
    static_assert(
        [] {
            for (std::size_t index = 0; index < kRoutes.size(); ++index) {
                if (kRoutes[index].shape != kTranslated[index]) {
                    return false;
                }
            }
            return true;
        }(),
        "each packet of kTranslated needs its translation, in the same order");
    for (const Route& route : kRoutes) {
        if (protocol::IsPacket(message, *route.shape)) {
            (this->*route.translate)(message, events);
            return true;
        }
    }
    return false;
}

void Translator::TranslateHero(Message message, world::EventQueue& events) {
    protocol::CharacterInformationBytes whole{};
    const auto character = protocol::ReadCharacterInformation(message, whole);
    if (!character) {
        Drop(dialect::CharacterInformation::kShape.name, character.error().message);
        return;
    }
    Put(events, world::HeroLoaded{
                    .id = m_hero,
                    .map = character->MapId(),
                    .position = Cell{character->X(), character->Y()},
                    .stats = {.health = character->CurrentHealth(),
                              .shield = character->CurrentShield(),
                              .mana = character->CurrentMana(),
                              .ability = character->CurrentAbility(),
                              .attackSpeed = character->AttackSpeed(),
                              .magicSpeed = character->MagicSpeed()},
                    .maxima = {.health = character->MaximumHealth(),
                               .shield = character->MaximumShield(),
                               .mana = character->MaximumMana(),
                               .ability = character->MaximumAbility()},
                });
}

void Translator::TranslateStats(Message message, world::EventQueue& events) {
    const auto stats = dialect::CurrentStats::View::Read(message);
    if (!stats) {
        Drop(dialect::CurrentStats::kShape.name, stats.error().message);
        return;
    }
    Put(events, world::HeroStatsChanged{.stats = {.health = stats->Health(),
                                                  .shield = stats->Shield(),
                                                  .mana = stats->Mana(),
                                                  .ability = stats->Ability(),
                                                  .attackSpeed = stats->AttackSpeed(),
                                                  .magicSpeed = stats->MagicSpeed()}});
}

void Translator::TranslatePlayer(Message message, world::EventQueue& events) {
    constexpr std::string_view kName = dialect::AddCharacterToScope::kShape.name;
    const auto player = dialect::AddCharacterToScope::View::Read(message);
    if (!player) {
        Drop(kName, player.error().message);
        return;
    }
    LogLine reason;
    if (message.size() < kPlayerBytes) {
        reason.Append("it has ").AppendNumber(message.size()).Append(" bytes, and a player needs at least ");
        Drop(kName, reason.AppendNumber(kPlayerBytes).View());
        return;
    }
    const std::uint8_t effectCount = protocol::ReadByte(message, kPlayerBytes - 1);
    if (message.size() < kPlayerBytes + std::size_t{effectCount}) {
        DescribeMissing(reason, effectCount, "effects", message.size() - kPlayerBytes);
        Drop(kName, reason.View());
        return;
    }
    const auto appearance = player->AppearanceAndEffects().first(protocol::kExtendedAppearanceSize);
    world::ObjectEntered entered;
    entered.id = IdOf(player->Id());
    entered.kind = world::ObjectKind::Player;
    entered.justAppeared = JustAppeared(player->Id());
    entered.position = Cell{player->CurrentPositionX(), player->CurrentPositionY()};
    entered.target = Cell{player->TargetPositionX(), player->TargetPositionY()};
    entered.rotation = player->Rotation();
    entered.name = world::ObjectName::FromBytes(player->Name().Bytes());
    entered.appearance = protocol::ReadExtendedAppearance(appearance);
    entered.effectCount = effectCount;
    Put(events, entered);
}

void Translator::TranslateNpcs(Message message, world::EventQueue& events) {
    constexpr std::string_view kName = dialect::AddNpcsToScope::kShape.name;
    const auto npcs = dialect::AddNpcsToScope::View::Read(message);
    if (!npcs) {
        Drop(kName, npcs.error().message);
        return;
    }
    // The whole list must be there before any NPC becomes an event, so a cut packet is dropped whole.
    const std::size_t count = npcs->NPCsCount();
    if (const auto missing =
            FirstMissing(count, [&](std::size_t index) { return npcs->NPCs(index).has_value(); })) {
        LogLine reason;
        DescribeMissing(reason, count, "NPCs or monsters", *missing);
        Drop(kName, reason.View());
        return;
    }
    for (std::size_t index = 0; index < count; ++index) {
        const auto npc = *npcs->NPCs(index);
        world::ObjectEntered entered;
        entered.id = IdOf(npc.Id());
        entered.kind = world::ObjectKind::NpcOrMonster;
        entered.justAppeared = JustAppeared(npc.Id());
        entered.position = Cell{npc.CurrentPositionX(), npc.CurrentPositionY()};
        entered.target = Cell{npc.TargetPositionX(), npc.TargetPositionY()};
        entered.rotation = npc.Rotation();
        entered.typeNumber = npc.TypeNumber();
        Put(events, entered);
    }
}

void Translator::TranslateLeaving(Message message, world::EventQueue& events) {
    constexpr std::string_view kName = dialect::MapObjectOutOfScope::kShape.name;
    const auto leaving = dialect::MapObjectOutOfScope::View::Read(message);
    if (!leaving) {
        Drop(kName, leaving.error().message);
        return;
    }
    const std::size_t count = leaving->ObjectsCount();
    if (const auto missing =
            FirstMissing(count, [&](std::size_t index) { return leaving->Objects(index).has_value(); })) {
        LogLine reason;
        DescribeMissing(reason, count, "objects", *missing);
        Drop(kName, reason.View());
        return;
    }
    for (std::size_t index = 0; index < count; ++index) {
        Put(events, world::ObjectLeft{.id = IdOf(leaving->Objects(index)->Id())});
    }
}

void Translator::TranslateWalk(Message message, world::EventQueue& events) {
    const auto walked = dialect::ObjectWalked::View::Read(message);
    if (!walked) {
        Drop(dialect::ObjectWalked::kShape.name, walked.error().message);
        return;
    }
    Put(events, world::ObjectStartedWalking{.id = IdOf(walked->ObjectId()),
                                            .source = Cell{walked->SourceX(), walked->SourceY()},
                                            .target = Cell{walked->TargetX(), walked->TargetY()},
                                            .rotation = walked->TargetRotation(),
                                            .stepCount = walked->StepCount()});
}

void Translator::TranslateCorrection(Message message, world::EventQueue& events) {
    const auto moved = dialect::ObjectMoved::View::Read(message);
    if (!moved) {
        Drop(dialect::ObjectMoved::kShape.name, moved.error().message);
        return;
    }
    Put(events, world::PositionCorrected{.id = IdOf(moved->ObjectId()),
                                         .position = Cell{moved->PositionX(), moved->PositionY()}});
}

void Translator::TranslateInventory(Message message, world::EventQueue& events) {
    constexpr std::string_view kName = dialect::CharacterInventory::kShape.name;
    const auto inventory = dialect::CharacterInventory::View::Read(message);
    if (!inventory) {
        Drop(kName, inventory.error().message);
        return;
    }
    const std::size_t count = inventory->ItemCount();
    const auto entries = inventory->ItemsBytes();
    const std::size_t whole = ForEachInventoryEntry(entries, count, [](std::uint8_t, const items::Item&) {});
    if (whole < count) {
        LogLine reason;
        DescribeMissing(reason, count, "items", whole);
        Drop(kName, reason.View());
        return;
    }
    Put(events, world::InventoryReplaced{.itemCount = inventory->ItemCount()});
    ForEachInventoryEntry(entries, count, [&](std::uint8_t slot, const items::Item& item) {
        Put(events, world::InventoryItemPut{.slot = slot, .item = item});
    });
}

void Translator::TranslateTerrain(Message message, world::EventQueue& events) {
    constexpr std::string_view kName = dialect::ChangeTerrainAttributes::kShape.name;
    const auto terrain = dialect::ChangeTerrainAttributes::View::Read(message);
    if (!terrain) {
        Drop(kName, terrain.error().message);
        return;
    }
    const std::size_t count = terrain->AreasCount();
    if (const auto missing =
            FirstMissing(count, [&](std::size_t index) { return terrain->Areas(index).has_value(); })) {
        LogLine reason;
        DescribeMissing(reason, count, "areas", *missing);
        Drop(kName, reason.View());
        return;
    }
    for (std::size_t index = 0; index < count; ++index) {
        const auto area = *terrain->Areas(index);
        Put(events, world::TerrainChanged{.start = Cell{area.StartX(), area.StartY()},
                                          .end = Cell{area.EndX(), area.EndY()},
                                          .attributes = static_cast<std::uint8_t>(terrain->Attribute()),
                                          .removed = terrain->RemoveAttribute()});
    }
}

void Translator::Put(world::EventQueue& events, const world::WorldEvent& event) {
    if (!events.Push(event) && !m_queueGrew) {
        m_queueGrew = true;
        m_log->Warning(kTranslate,
                       "the events of the world passed the reserved room; the queue grows, because "
                       "none may be dropped");
    }
}

void Translator::Drop(std::string_view packet, std::string_view reason) noexcept {
    LogLine line;
    line.Append("dropped the packet ").Append(packet).Append(": ").Append(reason);
    m_log->Warning(kTranslate, line.View());
}

void Translator::ReportUnknown(Message message) noexcept {
    if (message.empty()) {
        return;
    }
    const auto header = protocol::ReadByte(message, 0);
    const bool shortLength = header == 0xC1 || header == 0xC3;
    const std::size_t codeAt = shortLength ? 2 : 3;
    if (message.size() <= codeAt) {
        return;
    }
    const auto code = protocol::ReadByte(message, codeAt);
    const bool withSubCode = HasSubCodes(code) && message.size() > codeAt + 1;
    const std::uint8_t subCode = withSubCode ? protocol::ReadByte(message, codeAt + 1) : std::uint8_t{0};
    const std::size_t key = (std::size_t{code} << 8U) | std::size_t{subCode};
    if (m_reported.test(key)) {
        return;
    }
    m_reported.set(key);
    LogLine line;
    line.Append("ignored a packet the client does not know, ").AppendHex(header, 2).Append(" ");
    line.AppendHex(code, 2);
    if (withSubCode) {
        line.Append(" ").AppendHex(subCode, 2);
    }
    line.Append(" of ")
        .AppendNumber(message.size())
        .Append(" bytes; the next ones are ignored without a word");
    m_log->Info(kTranslate, line.View());
}

} // namespace mu::translate
