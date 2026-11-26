#include "Mu/World/WorldView.hpp"

#include <algorithm>

#include "Nexum/Debug/LogLine.hpp"

namespace mu::world {

namespace {

using nexum::debug::LogLevel;
using nexum::debug::LogLine;

constexpr nexum::debug::LogCategory kWorld{"world"};

// The room of the objects in view, reserved once. It is the Nexum's own number: the original sets no limit
// to the view, and a view that passes it grows, with a warning, instead of losing an object.
constexpr std::size_t kReservedObjects = 512;
// One entry for each value of the slot byte (section 6.12 of the protocol spec), so the inventory never
// grows.
constexpr std::size_t kInventorySlots = 256;

void AppendCell(LogLine& line, Cell cell) noexcept {
    line.Append("(").AppendNumber(cell.x).Append(", ").AppendNumber(cell.y).Append(")");
}

void AppendId(LogLine& line, ObjectId id) noexcept {
    line.Append("0x").AppendHex(id.value, 4);
}

// "the player test0Dk (0x0200, the hero)" or "the NPC or monster of type 14 (0x0021)".
void AppendObject(LogLine& line, const WorldObject& object, const Hero& hero) noexcept {
    if (object.kind == ObjectKind::Player) {
        line.Append("the player ").Append(object.name.View());
    } else {
        line.Append("the NPC or monster of type ").AppendNumber(object.typeNumber);
    }
    line.Append(" (");
    AppendId(line, object.id);
    line.Append(hero.loaded && object.id == hero.id ? ", the hero)" : ")");
}

void AppendStats(LogLine& line, const HeroStats& stats) noexcept {
    line.Append("health ").AppendNumber(stats.health).Append(", mana ").AppendNumber(stats.mana);
    line.Append(", shield ").AppendNumber(stats.shield).Append(", ability ").AppendNumber(stats.ability);
    line.Append(", attack speed ").AppendNumber(stats.attackSpeed);
    line.Append(", magic speed ").AppendNumber(stats.magicSpeed);
}

[[nodiscard]] WorldObject ObjectOf(const ObjectEntered& event) noexcept {
    WorldObject object;
    object.id = event.id;
    object.kind = event.kind;
    object.position = event.position;
    object.target = event.target;
    object.rotation = event.rotation;
    object.name = event.name;
    object.appearance = event.appearance;
    object.effectCount = event.effectCount;
    object.typeNumber = event.typeNumber;
    return object;
}

} // namespace

WorldView::WorldView(nexum::debug::Logger& log) : m_log(&log) {
    m_objects.reserve(kReservedObjects);
    m_inventory.reserve(kInventorySlots);
}

void WorldView::Apply(const WorldEvent& event) {
    std::visit([this](const auto& value) { On(value); }, event);
}

const WorldObject* WorldView::Find(ObjectId id) const noexcept {
    const auto found = std::ranges::find(m_objects, id, &WorldObject::id);
    return found == m_objects.end() ? nullptr : &*found;
}

WorldObject* WorldView::FindObject(ObjectId id) noexcept {
    const auto found = std::ranges::find(m_objects, id, &WorldObject::id);
    return found == m_objects.end() ? nullptr : &*found;
}

WorldObject* WorldView::FindOrIgnore(ObjectId id, std::string_view what) noexcept {
    if (WorldObject* object = FindObject(id)) {
        return object;
    }
    ++m_ignoredEvents;
    LogLine line;
    line.Append("ignored ").Append(what).Append(" of the object ");
    AppendId(line, id);
    line.Append(", which is not in view");
    Log(LogLevel::Warning, line.View());
    return nullptr;
}

void WorldView::On(const HeroLoaded& event) {
    m_hero = Hero{.loaded = true,
                  .id = event.id,
                  .map = event.map,
                  .position = event.position,
                  .stats = event.stats,
                  .maxima = event.maxima};
    LogLine line;
    line.Append("the hero ");
    AppendId(line, event.id);
    line.Append(" is on the map ").AppendNumber(event.map).Append(" at ");
    AppendCell(line, event.position);
    line.Append(", with health ").AppendNumber(event.stats.health).Append(" of ");
    line.AppendNumber(event.maxima.health).Append(" and mana ").AppendNumber(event.stats.mana).Append(" of ");
    line.AppendNumber(event.maxima.mana);
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const HeroStatsChanged& event) {
    if (event.stats == m_hero.stats) {
        return;
    }
    m_hero.stats = event.stats;
    LogLine line;
    line.Append("the hero has ");
    AppendStats(line, event.stats);
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const ObjectEntered& event) {
    const WorldObject object = ObjectOf(event);
    Keep(object);
    FollowHero(object);
    LogLine line;
    AppendObject(line, object, m_hero);
    line.Append(event.justAppeared ? " appeared at " : " came into view at ");
    AppendCell(line, object.position);
    if (object.Walking()) {
        line.Append(", walking to ");
        AppendCell(line, object.target);
    }
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const ObjectLeft& event) {
    const auto found = std::ranges::find(m_objects, event.id, &WorldObject::id);
    if (found == m_objects.end()) {
        static_cast<void>(FindOrIgnore(event.id, "the leaving"));
        return;
    }
    LogLine line;
    AppendObject(line, *found, m_hero);
    line.Append(" left the view");
    // The order of the table does not matter, so the last object takes the place of the one that left.
    *found = m_objects.back();
    m_objects.pop_back();
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const ObjectStartedWalking& event) {
    WorldObject* object = FindOrIgnore(event.id, "the walk");
    if (object == nullptr) {
        return;
    }
    object->position = event.source;
    object->target = event.target;
    object->rotation = event.rotation;
    FollowHero(*object);
    LogLine line;
    AppendObject(line, *object, m_hero);
    line.Append(" walks from ");
    AppendCell(line, event.source);
    line.Append(" to ");
    AppendCell(line, event.target);
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const PositionCorrected& event) {
    WorldObject* object = FindOrIgnore(event.id, "the correction");
    if (object == nullptr) {
        return;
    }
    object->position = event.position;
    object->target = event.position;
    FollowHero(*object);
    LogLine line;
    line.Append("the position of ");
    AppendObject(line, *object, m_hero);
    line.Append(" was corrected to ");
    AppendCell(line, event.position);
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const InventoryReplaced& event) {
    m_inventory.clear();
    LogLine line;
    line.Append("the inventory came with ").AppendNumber(event.itemCount).Append(" items");
    Log(LogLevel::Info, line.View());
}

void WorldView::On(const InventoryItemPut& event) {
    const auto found = std::ranges::find(m_inventory, event.slot, &InventoryEntry::slot);
    if (found != m_inventory.end()) {
        found->item = event.item;
    } else {
        m_inventory.push_back({event.slot, event.item});
    }
    if (m_log->IsEnabled(LogLevel::Debug)) {
        LogLine line;
        line.Append("the slot ").AppendNumber(event.slot).Append(" of the inventory has the item of group ");
        line.AppendNumber(event.item.group).Append(" and number ").AppendNumber(event.item.number);
        line.Append(", level ").AppendNumber(event.item.level);
        Log(LogLevel::Debug, line.View());
    }
}

void WorldView::On(const TerrainChanged& event) {
    ++m_terrainChanges;
    LogLine line;
    line.Append("the terrain from ");
    AppendCell(line, event.start);
    line.Append(" to ");
    AppendCell(line, event.end);
    line.Append(event.removed ? " loses the attributes 0x" : " gains the attributes 0x");
    line.AppendHex(event.attributes, 2);
    Log(LogLevel::Info, line.View());
}

void WorldView::FollowHero(const WorldObject& object) noexcept {
    if (m_hero.loaded && object.id == m_hero.id) {
        m_hero.position = object.position;
    }
}

void WorldView::Keep(const WorldObject& object) {
    if (WorldObject* known = FindObject(object.id)) {
        // The server sends an object again when it comes back into view; the new data is the truth.
        *known = object;
        return;
    }
    if (m_objects.size() == m_objects.capacity() && !m_objectsGrew) {
        m_objectsGrew = true;
        Log(LogLevel::Warning, "the objects in view passed the reserved room; the table grows");
    }
    m_objects.push_back(object);
}

void WorldView::Log(LogLevel level, std::string_view line) noexcept {
    m_log->Write(level, kWorld, line);
}

} // namespace mu::world
