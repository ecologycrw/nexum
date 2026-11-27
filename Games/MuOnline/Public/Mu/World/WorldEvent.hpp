#pragma once

// The neutral events of the world: what the server said, without the bytes it came in. They are the stable
// contract between the protocol and the systems of the game (IV.14.4 of the reference spec): the translation
// (Mu/Translate) makes them from the packets, the replay of slice 2 makes the same ones, and the view of the
// world, and later the systems of M4 and M5, take them one by one, in the order they came (INV-09). An event
// is a plain value of fixed size, so holding one never asks for memory.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

#include "Mu/Characters/Appearance.hpp"
#include "Mu/Items/Item.hpp"

namespace mu::world {

// The id the server gives an object of the map, without the mark of "just appeared".
struct ObjectId {
    std::uint16_t value = 0;

    friend constexpr bool operator==(ObjectId, ObjectId) noexcept = default;
};

// A cell of the grid of the map.
struct Cell {
    std::uint8_t x = 0;
    std::uint8_t y = 0;

    friend constexpr bool operator==(Cell, Cell) noexcept = default;
};

// A name as the server sends it: up to 10 bytes of UTF-8, kept in place.
class ObjectName {
public:
    static constexpr std::size_t kLongest = 10;

    ObjectName() = default;

    // The bytes of the name, up to the first zero; only the first 10 are kept.
    [[nodiscard]] static ObjectName FromBytes(std::span<const std::byte> bytes) noexcept;

    [[nodiscard]] std::string_view View() const noexcept { return {m_text.data(), m_size}; }
    [[nodiscard]] bool Empty() const noexcept { return m_size == 0; }

    friend bool operator==(const ObjectName&, const ObjectName&) = default;

private:
    std::array<char, kLongest> m_text{};
    std::size_t m_size = 0;
};

// The values of the hero that the server keeps up to date.
struct HeroStats {
    std::uint32_t health = 0;
    std::uint32_t shield = 0;
    std::uint32_t mana = 0;
    std::uint32_t ability = 0;
    std::uint16_t attackSpeed = 0;
    std::uint16_t magicSpeed = 0;

    friend bool operator==(const HeroStats&, const HeroStats&) = default;
};

struct HeroMaxima {
    std::uint32_t health = 0;
    std::uint32_t shield = 0;
    std::uint32_t mana = 0;
    std::uint32_t ability = 0;

    friend bool operator==(const HeroMaxima&, const HeroMaxima&) = default;
};

// The hero is on its map, with its values: the data of the character, which comes first.
struct HeroLoaded {
    ObjectId id;
    std::uint16_t map = 0;
    Cell position;
    HeroStats stats;
    HeroMaxima maxima;
};

struct HeroStatsChanged {
    HeroStats stats;
};

// NPCs and monsters come in the same packet; the number of the type tells them apart, from the data of M4 on.
enum class ObjectKind : std::uint8_t { Player, NpcOrMonster };

// An object came into the view: a player, the hero included, or an NPC or a monster.
struct ObjectEntered {
    ObjectId id;
    ObjectKind kind = ObjectKind::Player;
    // The object came into being just now, instead of walking into the view.
    bool justAppeared = false;
    Cell position;
    // The same cell as the position when the object stands; another one when it walks.
    Cell target;
    std::uint8_t rotation = 0;
    // Players only.
    ObjectName name;
    characters::Appearance appearance;
    // How many visible effects the player has; their numbers come with the effects of M5 (TD-0031).
    std::uint8_t effectCount = 0;
    // NPCs and monsters only.
    std::uint16_t typeNumber = 0;
};

struct ObjectLeft {
    ObjectId id;
};

// An object began to walk from one cell to another. For the hero, it is the echo of its own walk.
struct ObjectStartedWalking {
    ObjectId id;
    Cell source;
    Cell target;
    // The rotation before the walk, because the server sends the echo before the first step.
    std::uint8_t rotation = 0;
    std::uint8_t stepCount = 0;
};

// The server put the object in this cell at once, instead of letting it walk.
struct PositionCorrected {
    ObjectId id;
    Cell position;
};

// The whole inventory came again; the items follow, one event each.
struct InventoryReplaced {
    std::uint8_t itemCount = 0;
};

struct InventoryItemPut {
    std::uint8_t slot = 0;
    items::Item item;
};

// The attributes of the cells of an area of the map, from `start` to `end`, gain or lose these bits.
struct TerrainChanged {
    Cell start;
    Cell end;
    std::uint8_t attributes = 0;
    bool removed = false;
};

using WorldEvent = std::variant<HeroLoaded, HeroStatsChanged, ObjectEntered, ObjectLeft, ObjectStartedWalking,
                                PositionCorrected, InventoryReplaced, InventoryItemPut, TerrainChanged>;

} // namespace mu::world
