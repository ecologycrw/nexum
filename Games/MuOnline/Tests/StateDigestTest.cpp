// The digest of the state of the view of the world, version 1 (CA-06): the order of the tables does not
// change it, and every field that matters does. The two fixed values were computed apart, from the list of
// fields of the plan of slice 2 ("A impressão do estado, versão 1"), by a script that shares no code with the
// client: they pin the order and the size of each field, which a recording keeps.
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Characters/Appearance.hpp"
#include "Mu/Items/Item.hpp"
#include "Mu/Recording/StateDigest.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Core/Fnv1a.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

namespace {

namespace characters = mu::characters;
namespace world = mu::world;
using mu::recording::StateDigestOf;

constexpr world::ObjectId kHero{0x0200};
constexpr world::ObjectId kPlayer{0x0201};
constexpr world::ObjectId kMonster{0x0021};
constexpr world::ObjectId kOtherMonster{0x0022};
// An object that is never in view, so an event about it is ignored.
constexpr world::ObjectId kAbsent{0x0333};

// The view with no event: its 83 bytes are all zero.
constexpr std::uint64_t kEmptyDigest = 0x24A21E4B6B29FC77ULL;
// The view of the scene below, with its events in any order.
constexpr std::uint64_t kSceneDigest = 0x5E8BDE3692700E3AULL;

// The 7 worn items, the wings and the pet.
constexpr std::size_t kWornItems = 9;

[[nodiscard]] world::ObjectName Name(std::string_view text) {
    std::array<std::byte, world::ObjectName::kLongest> bytes{};
    for (std::size_t index = 0; index < text.size() && index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(text[index]);
    }
    return world::ObjectName::FromBytes(bytes);
}

[[nodiscard]] characters::WornItem& WornAt(characters::Appearance& appearance, std::size_t index) {
    if (index < appearance.worn.size()) {
        return appearance.worn[index];
    }
    return index == appearance.worn.size() ? appearance.wings : appearance.pet;
}

[[nodiscard]] world::ObjectEntered Player() {
    world::ObjectEntered entered;
    entered.id = kPlayer;
    entered.kind = world::ObjectKind::Player;
    entered.position = {146, 124};
    entered.target = {147, 125};
    entered.rotation = 2;
    entered.name = Name("test1Dk");
    entered.appearance.characterClass = 4;
    entered.appearance.pose = 2;
    entered.appearance.worn[static_cast<std::size_t>(characters::WornSlot::Armor)] = characters::WornItem{
        .present = true, .group = 8, .number = 1, .glow = 2, .excellent = true, .ancient = false};
    entered.appearance.wings = characters::WornItem{
        .present = true, .group = 12, .number = 3, .glow = 0, .excellent = false, .ancient = false};
    entered.appearance.fenrir = characters::FenrirVariant::Blue;
    entered.effectCount = 1;
    return entered;
}

[[nodiscard]] world::ObjectEntered Monster(world::ObjectId id) {
    world::ObjectEntered entered;
    entered.id = id;
    entered.kind = world::ObjectKind::NpcOrMonster;
    entered.justAppeared = true;
    entered.position = {135, 124};
    entered.target = {135, 124};
    entered.rotation = 3;
    entered.typeNumber = 14;
    return entered;
}

[[nodiscard]] world::InventoryItemPut Sword() {
    mu::items::Item item;
    item.number = 1;
    item.level = 3;
    item.durability = 22;
    item.hasOption = true;
    item.optionLevel = 1;
    item.luck = true;
    item.skill = true;
    item.excellent = 0x05;
    return {.slot = 12, .item = item};
}

[[nodiscard]] world::InventoryItemPut SocketedItem() {
    mu::items::Item item;
    item.number = 26;
    item.durability = 80;
    item.hasSockets = true;
    item.socketCount = 3;
    item.sockets[0] = mu::items::kEmptySocket;
    item.sockets[1] = 0x11;
    item.sockets[2] = mu::items::kEmptySocket;
    return {.slot = 0, .item = item};
}

// A hero, a player walking and a monster in view, two entries of the inventory, a change of the terrain and
// an ignored event: every part of the digest has something that is not zero.
struct Scene {
    world::HeroLoaded hero{
        .id = kHero,
        .map = 0,
        .position = {145, 123},
        .stats = {.health = 110, .shield = 5, .mana = 20, .ability = 7, .attackSpeed = 29, .magicSpeed = 30},
        .maxima = {.health = 120, .shield = 6, .mana = 30, .ability = 8}};
    world::ObjectEntered player = Player();
    world::ObjectEntered monster = Monster(kMonster);
    world::InventoryItemPut sword = Sword();
    world::InventoryItemPut socketed = SocketedItem();
    int terrainChanges = 1;
    int ignoredEvents = 1;
    // A third object and a third entry, for the counts.
    bool otherMonster = false;
    bool thirdEntry = false;
};

struct View {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink, nexum::debug::LogLevel::Info};
    world::WorldView view{log};

    void Apply(const world::WorldEvent& event) { view.Apply(event); }

    void Apply(std::initializer_list<world::WorldEvent> events) {
        for (const auto& event : events) {
            view.Apply(event);
        }
    }
};

[[nodiscard]] world::TerrainChanged TerrainChange() {
    return {.start = {10, 10}, .end = {12, 12}, .attributes = 0x04, .removed = false};
}

[[nodiscard]] std::uint64_t DigestOf(const Scene& scene) {
    View view;
    view.Apply({scene.hero, scene.player, scene.monster, scene.sword, scene.socketed});
    if (scene.otherMonster) {
        view.Apply(Monster(kOtherMonster));
    }
    if (scene.thirdEntry) {
        world::InventoryItemPut entry = Sword();
        entry.slot = 20;
        view.Apply(entry);
    }
    for (int change = 0; change < scene.terrainChanges; ++change) {
        view.Apply(TerrainChange());
    }
    for (int ignored = 0; ignored < scene.ignoredEvents; ++ignored) {
        view.Apply(world::ObjectLeft{.id = kAbsent});
    }
    return StateDigestOf(view.view);
}

struct Change {
    std::string what;
    std::function<void(Scene&)> apply;
};

// Each change, alone, gives a digest other than the one of the scene. The fixed value of the scene has a test
// of its own, so a field left out of the digest fails here by its name.
void CheckEachChangeMatters(const std::vector<Change>& changes) {
    const std::uint64_t unchanged = DigestOf(Scene{});
    for (const Change& change : changes) {
        Scene scene;
        change.apply(scene);
        CHECK_MESSAGE(DigestOf(scene) != unchanged, change.what);
    }
}

void Toggle(bool& value) noexcept {
    value = !value;
}

[[nodiscard]] std::vector<Change> WornItemChanges() {
    std::vector<Change> changes;
    for (std::size_t index = 0; index < kWornItems; ++index) {
        const std::string where = "worn item " + std::to_string(index) + ": ";
        const auto worn = [index](Scene& scene) -> characters::WornItem& {
            return WornAt(scene.player.appearance, index);
        };
        changes.push_back({where + "present", [worn](Scene& scene) { Toggle(worn(scene).present); }});
        changes.push_back({where + "group", [worn](Scene& scene) { ++worn(scene).group; }});
        changes.push_back({where + "number", [worn](Scene& scene) { ++worn(scene).number; }});
        changes.push_back({where + "glow", [worn](Scene& scene) { ++worn(scene).glow; }});
        changes.push_back({where + "excellent", [worn](Scene& scene) { Toggle(worn(scene).excellent); }});
        changes.push_back({where + "ancient", [worn](Scene& scene) { Toggle(worn(scene).ancient); }});
    }
    return changes;
}

} // namespace

TEST_CASE("[0002-CA-06] the digest of an empty view is the FNV-1a of its 83 bytes, all zero") {
    View view;
    CHECK(StateDigestOf(view.view) == kEmptyDigest);
    CHECK(nexum::core::Fnv1a64Of(std::array<std::byte, 83>{}) == kEmptyDigest);
}

TEST_CASE("[0002-CA-06] the digest of a view with every part filled has the value computed apart") {
    CHECK(DigestOf(Scene{}) == kSceneDigest);
}

TEST_CASE("[0002-CA-06] the order of the objects and of the inventory does not change the digest") {
    const Scene scene;
    View reversed;
    reversed.Apply({world::ObjectLeft{.id = kAbsent}, TerrainChange(), scene.hero, scene.socketed,
                    scene.monster, scene.sword, scene.player});
    REQUIRE(reversed.view.Objects().size() == 2);
    CHECK(reversed.view.Objects()[0].id == kMonster);
    REQUIRE(reversed.view.Inventory().size() == 2);
    CHECK(reversed.view.Inventory()[0].slot == 0);
    CHECK(StateDigestOf(reversed.view) == kSceneDigest);
}

TEST_CASE("[0002-CA-06] an object that leaves the view reorders the table without changing the digest") {
    const Scene scene;
    View left;
    left.Apply(
        {scene.hero, scene.player, scene.monster, Monster(kOtherMonster), world::ObjectLeft{.id = kPlayer}});
    // The last object took the place of the one that left.
    REQUIRE(left.view.Objects().size() == 2);
    CHECK(left.view.Objects()[0].id == kOtherMonster);

    View direct;
    direct.Apply({scene.hero, scene.monster, Monster(kOtherMonster)});
    REQUIRE(direct.view.Objects().size() == 2);
    CHECK(direct.view.Objects()[0].id == kMonster);
    CHECK(StateDigestOf(left.view) == StateDigestOf(direct.view));
}

TEST_CASE("[0002-CA-06] the hero that was loaded changes the digest, even with every value at zero") {
    View empty;
    View loaded;
    loaded.Apply(world::HeroLoaded{});
    CHECK(StateDigestOf(loaded.view) != StateDigestOf(empty.view));
}

TEST_CASE("[0002-CA-06] changing any value of the hero changes the digest") {
    CheckEachChangeMatters({
        // Down, because the id above is the player's, whose position the hero would then follow.
        {"id", [](Scene& scene) { --scene.hero.id.value; }},
        {"map", [](Scene& scene) { ++scene.hero.map; }},
        {"position x", [](Scene& scene) { ++scene.hero.position.x; }},
        {"position y", [](Scene& scene) { ++scene.hero.position.y; }},
        {"health", [](Scene& scene) { ++scene.hero.stats.health; }},
        {"shield", [](Scene& scene) { ++scene.hero.stats.shield; }},
        {"mana", [](Scene& scene) { ++scene.hero.stats.mana; }},
        {"ability", [](Scene& scene) { ++scene.hero.stats.ability; }},
        {"attack speed", [](Scene& scene) { ++scene.hero.stats.attackSpeed; }},
        {"magic speed", [](Scene& scene) { ++scene.hero.stats.magicSpeed; }},
        {"most health", [](Scene& scene) { ++scene.hero.maxima.health; }},
        {"most shield", [](Scene& scene) { ++scene.hero.maxima.shield; }},
        {"most mana", [](Scene& scene) { ++scene.hero.maxima.mana; }},
        {"most ability", [](Scene& scene) { ++scene.hero.maxima.ability; }},
    });
}

TEST_CASE("[0002-CA-06] changing any field of an object in view changes the digest") {
    std::vector<Change> changes{
        {"id", [](Scene& scene) { ++scene.player.id.value; }},
        {"kind", [](Scene& scene) { scene.player.kind = world::ObjectKind::NpcOrMonster; }},
        {"position x", [](Scene& scene) { ++scene.player.position.x; }},
        {"position y", [](Scene& scene) { ++scene.player.position.y; }},
        {"target x", [](Scene& scene) { ++scene.player.target.x; }},
        {"target y", [](Scene& scene) { ++scene.player.target.y; }},
        {"rotation", [](Scene& scene) { ++scene.player.rotation; }},
        {"a byte of the name", [](Scene& scene) { scene.player.name = Name("test1Dl"); }},
        {"the size of the name", [](Scene& scene) { scene.player.name = Name("test1D"); }},
        {"class", [](Scene& scene) { ++scene.player.appearance.characterClass; }},
        {"pose", [](Scene& scene) { ++scene.player.appearance.pose; }},
        {"full ancient set", [](Scene& scene) { Toggle(scene.player.appearance.fullAncientSet); }},
        {"game master", [](Scene& scene) { Toggle(scene.player.appearance.gameMaster); }},
        {"fenrir", [](Scene& scene) { scene.player.appearance.fenrir = characters::FenrirVariant::Gold; }},
        {"effects", [](Scene& scene) { ++scene.player.effectCount; }},
        {"type number", [](Scene& scene) { ++scene.player.typeNumber; }},
    };
    for (Change& change : WornItemChanges()) {
        changes.push_back(std::move(change));
    }
    CheckEachChangeMatters(changes);
}

TEST_CASE("[0002-CA-06] changing any field of an entry of the inventory changes the digest") {
    std::vector<Change> changes{
        {"slot", [](Scene& scene) { ++scene.sword.slot; }},
        {"group", [](Scene& scene) { ++scene.sword.item.group; }},
        {"number", [](Scene& scene) { ++scene.sword.item.number; }},
        {"level", [](Scene& scene) { ++scene.sword.item.level; }},
        {"durability", [](Scene& scene) { ++scene.sword.item.durability; }},
        {"has option", [](Scene& scene) { Toggle(scene.sword.item.hasOption); }},
        {"option type", [](Scene& scene) { ++scene.sword.item.optionType; }},
        {"option level", [](Scene& scene) { ++scene.sword.item.optionLevel; }},
        {"luck", [](Scene& scene) { Toggle(scene.sword.item.luck); }},
        {"skill", [](Scene& scene) { Toggle(scene.sword.item.skill); }},
        {"excellent", [](Scene& scene) { ++scene.sword.item.excellent; }},
        {"has ancient", [](Scene& scene) { Toggle(scene.sword.item.hasAncient); }},
        {"ancient set", [](Scene& scene) { ++scene.sword.item.ancientSet; }},
        {"ancient bonus level", [](Scene& scene) { ++scene.sword.item.ancientBonusLevel; }},
        {"has harmony", [](Scene& scene) { Toggle(scene.sword.item.hasHarmony); }},
        {"harmony number", [](Scene& scene) { ++scene.sword.item.harmonyNumber; }},
        {"harmony level", [](Scene& scene) { ++scene.sword.item.harmonyLevel; }},
        {"option 380", [](Scene& scene) { Toggle(scene.sword.item.option380); }},
        {"has sockets", [](Scene& scene) { Toggle(scene.socketed.item.hasSockets); }},
        {"socket bonus", [](Scene& scene) { ++scene.socketed.item.socketBonus; }},
        {"socket count", [](Scene& scene) { ++scene.socketed.item.socketCount; }},
    };
    for (std::size_t index = 0; index < mu::items::Item{}.sockets.size(); ++index) {
        changes.push_back({"socket " + std::to_string(index),
                           [index](Scene& scene) { ++scene.socketed.item.sockets[index]; }});
    }
    CheckEachChangeMatters(changes);
}

TEST_CASE("[0002-CA-06] the counts of objects, entries, changes of the terrain and ignored events change the "
          "digest") {
    CheckEachChangeMatters({
        {"a third object", [](Scene& scene) { scene.otherMonster = true; }},
        {"a third entry", [](Scene& scene) { scene.thirdEntry = true; }},
        {"no change of the terrain", [](Scene& scene) { scene.terrainChanges = 0; }},
        {"two changes of the terrain", [](Scene& scene) { scene.terrainChanges = 2; }},
        {"no ignored event", [](Scene& scene) { scene.ignoredEvents = 0; }},
        {"two ignored events", [](Scene& scene) { scene.ignoredEvents = 2; }},
    });
}
