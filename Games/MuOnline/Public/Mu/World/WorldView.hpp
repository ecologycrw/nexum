#pragma once

// What the client knows of the world, built only from the neutral events (Mu/World/WorldEvent.hpp), one by
// one, inside the tick: the hero, the objects in view, the inventory and the changes of the terrain. The
// server decides everything here (INV-01): an event is the truth, and the view never guesses. It knows no
// packet and no network, so its tests feed it events alone.
//
// For now it is a table by id, without entities, because nothing is drawn or moves yet; the objects become
// entities of Engine/World, with the double coordinate of the kit, in M4 (TD-0029). The log tells each
// change, and the lines are built in place, so applying an event never asks for memory while the reserved
// room lasts (INV-13).

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "Mu/World/WorldEvent.hpp"
#include "Nexum/Debug/Logger.hpp"

namespace mu::world {

struct WorldObject {
    ObjectId id;
    ObjectKind kind = ObjectKind::Player;
    Cell position;
    // The cell the object walks to; the same as the position when it stands.
    Cell target;
    std::uint8_t rotation = 0;
    ObjectName name;                   // players
    characters::Appearance appearance; // players
    std::uint8_t effectCount = 0;      // players
    std::uint16_t typeNumber = 0;      // NPCs and monsters

    [[nodiscard]] bool Walking() const noexcept { return !(position == target); }
};

struct Hero {
    bool loaded = false;
    ObjectId id;
    std::uint16_t map = 0;
    Cell position;
    HeroStats stats;
    HeroMaxima maxima;
};

struct InventoryEntry {
    std::uint8_t slot = 0;
    items::Item item;
};

class WorldView {
public:
    // The log must outlive the view.
    explicit WorldView(nexum::debug::Logger& log);

    // Applies one event. An event about an object that is not in view is ignored and logged (IV.14.6 of the
    // reference spec): the view keeps no guess of where it might be.
    void Apply(const WorldEvent& event);

    [[nodiscard]] const Hero& HeroState() const noexcept { return m_hero; }
    [[nodiscard]] std::span<const WorldObject> Objects() const noexcept { return m_objects; }
    // The object with this id, or null when it is not in view. The pointer lasts until the next event.
    [[nodiscard]] const WorldObject* Find(ObjectId id) const noexcept;
    [[nodiscard]] std::span<const InventoryEntry> Inventory() const noexcept { return m_inventory; }
    [[nodiscard]] std::uint64_t TerrainChanges() const noexcept { return m_terrainChanges; }
    // How many events were about an object that was not in view.
    [[nodiscard]] std::uint64_t IgnoredEvents() const noexcept { return m_ignoredEvents; }

private:
    void On(const HeroLoaded& event);
    void On(const HeroStatsChanged& event);
    void On(const ObjectEntered& event);
    void On(const ObjectLeft& event);
    void On(const ObjectStartedWalking& event);
    void On(const PositionCorrected& event);
    void On(const InventoryReplaced& event);
    void On(const InventoryItemPut& event);
    void On(const TerrainChanged& event);

    [[nodiscard]] WorldObject* FindObject(ObjectId id) noexcept;
    // The object, or null after logging that the event about it, which `what` names, was ignored.
    [[nodiscard]] WorldObject* FindOrIgnore(ObjectId id, std::string_view what) noexcept;
    // Keeps the position of the hero in step with its object in view.
    void FollowHero(const WorldObject& object) noexcept;
    void Keep(const WorldObject& object);
    void Log(nexum::debug::LogLevel level, std::string_view line) noexcept;

    nexum::debug::Logger* m_log;
    Hero m_hero;
    std::vector<WorldObject> m_objects;
    std::vector<InventoryEntry> m_inventory;
    std::uint64_t m_terrainChanges = 0;
    std::uint64_t m_ignoredEvents = 0;
    bool m_objectsGrew = false;
};

} // namespace mu::world
