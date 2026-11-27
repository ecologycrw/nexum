#include "Mu/Recording/StateDigest.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <span>
#include <string_view>
#include <type_traits>

#include "Mu/Characters/Appearance.hpp"
#include "Mu/Items/Item.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Nexum/Core/Fnv1a.hpp"

namespace mu::recording {

namespace {

using nexum::core::Fnv1a64;

void AddFlag(Fnv1a64& hash, bool value) noexcept {
    hash.AddByte(value ? std::byte{1} : std::byte{0});
}

template <typename Enum>
    requires std::is_enum_v<Enum>
void AddEnum(Fnv1a64& hash, Enum value) noexcept {
    hash.AddLittleEndian(static_cast<std::underlying_type_t<Enum>>(value));
}

void AddCell(Fnv1a64& hash, world::Cell cell) noexcept {
    hash.AddLittleEndian(cell.x);
    hash.AddLittleEndian(cell.y);
}

void AddHero(Fnv1a64& hash, const world::Hero& hero) noexcept {
    AddFlag(hash, hero.loaded);
    hash.AddLittleEndian(hero.id.value);
    hash.AddLittleEndian(hero.map);
    AddCell(hash, hero.position);
    const world::HeroStats& stats = hero.stats;
    hash.AddLittleEndian(stats.health);
    hash.AddLittleEndian(stats.shield);
    hash.AddLittleEndian(stats.mana);
    hash.AddLittleEndian(stats.ability);
    hash.AddLittleEndian(stats.attackSpeed);
    hash.AddLittleEndian(stats.magicSpeed);
    const world::HeroMaxima& maxima = hero.maxima;
    hash.AddLittleEndian(maxima.health);
    hash.AddLittleEndian(maxima.shield);
    hash.AddLittleEndian(maxima.mana);
    hash.AddLittleEndian(maxima.ability);
}

// The size in one byte, because a name keeps at most 10, and then the bytes: without the size, "ab" followed
// by "c" would give the same digest as "abc".
void AddName(Fnv1a64& hash, const world::ObjectName& name) noexcept {
    const std::string_view text = name.View();
    hash.AddLittleEndian(static_cast<std::uint8_t>(text.size()));
    hash.Add(text);
}

void AddWornItem(Fnv1a64& hash, const characters::WornItem& item) noexcept {
    AddFlag(hash, item.present);
    hash.AddLittleEndian(item.group);
    hash.AddLittleEndian(item.number);
    hash.AddLittleEndian(item.glow);
    AddFlag(hash, item.excellent);
    AddFlag(hash, item.ancient);
}

void AddAppearance(Fnv1a64& hash, const characters::Appearance& appearance) noexcept {
    hash.AddLittleEndian(appearance.characterClass);
    hash.AddLittleEndian(appearance.pose);
    AddFlag(hash, appearance.fullAncientSet);
    AddFlag(hash, appearance.gameMaster);
    for (const characters::WornItem& item : appearance.worn) {
        AddWornItem(hash, item);
    }
    AddWornItem(hash, appearance.wings);
    AddWornItem(hash, appearance.pet);
    AddEnum(hash, appearance.fenrir);
}

[[nodiscard]] std::uint64_t ObjectDigest(const world::WorldObject& object) noexcept {
    Fnv1a64 hash;
    hash.AddLittleEndian(object.id.value);
    AddEnum(hash, object.kind);
    AddCell(hash, object.position);
    AddCell(hash, object.target);
    hash.AddLittleEndian(object.rotation);
    AddName(hash, object.name);
    AddAppearance(hash, object.appearance);
    hash.AddLittleEndian(object.effectCount);
    hash.AddLittleEndian(object.typeNumber);
    return hash.Value();
}

void AddItem(Fnv1a64& hash, const items::Item& item) noexcept {
    hash.AddLittleEndian(item.group);
    hash.AddLittleEndian(item.number);
    hash.AddLittleEndian(item.level);
    hash.AddLittleEndian(item.durability);
    AddFlag(hash, item.hasOption);
    hash.AddLittleEndian(item.optionType);
    hash.AddLittleEndian(item.optionLevel);
    AddFlag(hash, item.luck);
    AddFlag(hash, item.skill);
    hash.AddLittleEndian(item.excellent);
    AddFlag(hash, item.hasAncient);
    hash.AddLittleEndian(item.ancientSet);
    hash.AddLittleEndian(item.ancientBonusLevel);
    AddFlag(hash, item.hasHarmony);
    hash.AddLittleEndian(item.harmonyNumber);
    hash.AddLittleEndian(item.harmonyLevel);
    AddFlag(hash, item.option380);
    AddFlag(hash, item.hasSockets);
    hash.AddLittleEndian(item.socketBonus);
    hash.AddLittleEndian(item.socketCount);
    for (const std::uint8_t socket : item.sockets) {
        hash.AddLittleEndian(socket);
    }
}

[[nodiscard]] std::uint64_t EntryDigest(const world::InventoryEntry& entry) noexcept {
    Fnv1a64 hash;
    hash.AddLittleEndian(entry.slot);
    AddItem(hash, entry.item);
    return hash.Value();
}

// The sum of the digests, modulo 2^64: adding does not care about the order.
template <typename Value>
[[nodiscard]] std::uint64_t SumOfDigests(std::span<const Value> values,
                                         std::uint64_t (*digest)(const Value&) noexcept) noexcept {
    return std::transform_reduce(values.begin(), values.end(), std::uint64_t{0}, std::plus<>{}, digest);
}

// The view keeps at most one object for each id of 2 bytes and one entry for each slot of 1 byte, so both
// counts fit in 4 bytes.
[[nodiscard]] std::uint32_t CountOf(std::size_t size) noexcept {
    return static_cast<std::uint32_t>(size);
}

} // namespace

std::uint64_t StateDigestOf(const world::WorldView& view) noexcept {
    Fnv1a64 hash;
    AddHero(hash, view.HeroState());
    hash.AddLittleEndian(SumOfDigests(view.Objects(), ObjectDigest));
    hash.AddLittleEndian(SumOfDigests(view.Inventory(), EntryDigest));
    hash.AddLittleEndian(CountOf(view.Objects().size()));
    hash.AddLittleEndian(CountOf(view.Inventory().size()));
    hash.AddLittleEndian(view.TerrainChanges());
    hash.AddLittleEndian(view.IgnoredEvents());
    return hash.Value();
}

} // namespace mu::recording
