// The view of the world, fed with neutral events alone, without packets and without a network (CA-09,
// INV-01). The cells and the ids are those of the examples of the plan of slice 1 ("Comportamento: os pacotes
// do mundo e os eventos").
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

namespace {

namespace world = mu::world;
using mu::session::SessionEpoch;
using nexum::debug::LogLevel;

constexpr world::ObjectId kHero{0x0200};
constexpr world::ObjectId kMonster{0x0021};

[[nodiscard]] world::ObjectName Name(std::string_view text) {
    std::array<std::byte, world::ObjectName::kLongest> bytes{};
    for (std::size_t index = 0; index < text.size() && index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(text[index]);
    }
    return world::ObjectName::FromBytes(bytes);
}

[[nodiscard]] world::HeroLoaded Hero() {
    return world::HeroLoaded{.id = kHero,
                             .map = 0,
                             .position = {145, 123},
                             .stats = {.health = 110, .mana = 20, .attackSpeed = 29, .magicSpeed = 29},
                             .maxima = {.health = 120, .mana = 30}};
}

[[nodiscard]] world::ObjectEntered Player(world::ObjectId id, std::string_view name, world::Cell position) {
    world::ObjectEntered entered;
    entered.id = id;
    entered.kind = world::ObjectKind::Player;
    entered.position = position;
    entered.target = position;
    entered.name = Name(name);
    return entered;
}

[[nodiscard]] world::ObjectEntered Monster(world::ObjectId id, std::uint16_t type, world::Cell position) {
    world::ObjectEntered entered;
    entered.id = id;
    entered.kind = world::ObjectKind::NpcOrMonster;
    entered.justAppeared = true;
    entered.position = position;
    entered.target = position;
    entered.rotation = 3;
    entered.typeNumber = type;
    return entered;
}

struct View {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink, LogLevel::Debug};
    world::WorldView view{log};

    void Apply(std::initializer_list<world::WorldEvent> events) {
        for (const auto& event : events) {
            view.Apply(event);
        }
    }

    [[nodiscard]] bool LogHas(std::string_view text, LogLevel level = LogLevel::Info) const {
        return std::ranges::any_of(
            sink.Entries(), [&](const auto& entry) { return entry.level == level && entry.message == text; });
    }
};

} // namespace

TEST_CASE("[0001-CA-09] the hero is loaded on its map, then sees itself come into view") {
    View view;
    view.Apply({Hero(), Player(kHero, "test0Dk", {145, 123})});
    const auto& hero = view.view.HeroState();
    CHECK(hero.loaded);
    CHECK(hero.id == kHero);
    CHECK(hero.map == 0);
    CHECK(hero.position == world::Cell{145, 123});
    CHECK(hero.stats.health == 110);
    CHECK(hero.maxima.health == 120);
    REQUIRE(view.view.Objects().size() == 1);
    const auto* object = view.view.Find(kHero);
    REQUIRE(object != nullptr);
    CHECK(object->name.View() == "test0Dk");
    CHECK_FALSE(object->Walking());
    CHECK(view.LogHas(
        "the hero 0x0200 is on the map 0 at (145, 123), with health 110 of 120 and mana 20 of 30"));
    CHECK(view.LogHas("the player test0Dk (0x0200, the hero) came into view at (145, 123)"));
}

TEST_CASE("[0001-CA-09] a monster appears, walks, has its position corrected and leaves the view") {
    View view;
    view.Apply({Hero(), Monster(kMonster, 14, {135, 124})});
    CHECK(view.LogHas("the NPC or monster of type 14 (0x0021) appeared at (135, 124)"));

    view.Apply({world::ObjectStartedWalking{.id = kMonster, .source = {135, 124}, .target = {137, 125}}});
    const auto* monster = view.view.Find(kMonster);
    REQUIRE(monster != nullptr);
    CHECK(monster->position == world::Cell{135, 124});
    CHECK(monster->target == world::Cell{137, 125});
    CHECK(monster->Walking());
    CHECK(view.LogHas("the NPC or monster of type 14 (0x0021) walks from (135, 124) to (137, 125)"));

    view.Apply({world::PositionCorrected{.id = kMonster, .position = {136, 124}}});
    monster = view.view.Find(kMonster);
    REQUIRE(monster != nullptr);
    CHECK(monster->position == world::Cell{136, 124});
    CHECK_FALSE(monster->Walking());
    CHECK(view.LogHas("the position of the NPC or monster of type 14 (0x0021) was corrected to (136, 124)"));

    view.Apply({world::ObjectLeft{.id = kMonster}});
    CHECK(view.view.Find(kMonster) == nullptr);
    CHECK(view.view.Objects().empty());
    CHECK(view.LogHas("the NPC or monster of type 14 (0x0021) left the view"));
    CHECK(view.view.IgnoredEvents() == 0);
}

TEST_CASE("[0001-CA-09] the position of the hero follows its walk and its correction") {
    View view;
    view.Apply({Hero(), Player(kHero, "test0Dk", {145, 123}),
                world::ObjectStartedWalking{
                    .id = kHero, .source = {145, 123}, .target = {144, 126}, .rotation = 3}});
    CHECK(view.view.HeroState().position == world::Cell{145, 123});
    CHECK(view.LogHas("the player test0Dk (0x0200, the hero) walks from (145, 123) to (144, 126)"));
    view.Apply({world::PositionCorrected{.id = kHero, .position = {144, 126}}});
    CHECK(view.view.HeroState().position == world::Cell{144, 126});
}

TEST_CASE("[0001-CA-09] an event about an object that is not in view is ignored and logged") {
    View view;
    view.Apply({Hero(), world::ObjectStartedWalking{.id = kMonster, .source = {1, 2}, .target = {3, 4}},
                world::PositionCorrected{.id = kMonster, .position = {5, 6}},
                world::ObjectLeft{.id = kMonster}});
    CHECK(view.view.Objects().empty());
    CHECK(view.view.IgnoredEvents() == 3);
    CHECK(view.LogHas("ignored the walk of the object 0x0021, which is not in view", LogLevel::Warning));
    CHECK(
        view.LogHas("ignored the correction of the object 0x0021, which is not in view", LogLevel::Warning));
    CHECK(view.LogHas("ignored the leaving of the object 0x0021, which is not in view", LogLevel::Warning));
}

TEST_CASE("[0001-CA-09] an object that comes into view again is replaced, not repeated") {
    View view;
    view.Apply({Monster(kMonster, 14, {135, 124}), Monster(kMonster, 14, {100, 100})});
    REQUIRE(view.view.Objects().size() == 1);
    CHECK(view.view.Objects()[0].position == world::Cell{100, 100});
}

TEST_CASE("[0001-CA-09] a player that comes into view walking shows where it goes") {
    View view;
    auto walking = Player(world::ObjectId{0x0201}, "test1Dk", {140, 120});
    walking.target = {142, 121};
    view.Apply({Hero(), walking});
    CHECK(view.LogHas("the player test1Dk (0x0201) came into view at (140, 120), walking to (142, 121)"));
}

TEST_CASE("[0001-CA-09] the stats of the hero change, and the log tells only a real change") {
    View view;
    const world::HeroStats changed{
        .health = 90, .shield = 0, .mana = 15, .ability = 4, .attackSpeed = 29, .magicSpeed = 29};
    view.Apply(
        {Hero(), world::HeroStatsChanged{.stats = changed}, world::HeroStatsChanged{.stats = changed}});
    CHECK(view.view.HeroState().stats == changed);
    CHECK(
        view.LogHas("the hero has health 90, mana 15, shield 0, ability 4, attack speed 29, magic speed 29"));
    CHECK(std::ranges::count_if(view.sink.Entries(), [](const auto& entry) {
              return entry.message.starts_with("the hero has");
          }) == 1);
}

TEST_CASE("[0001-CA-09] the inventory is replaced whole, and each item goes to its slot") {
    View view;
    mu::items::Item sword;
    sword.number = 22;
    sword.level = 9;
    mu::items::Item gloves;
    gloves.group = 10;
    gloves.number = 5;
    view.Apply({world::InventoryReplaced{.itemCount = 2}, world::InventoryItemPut{.slot = 0, .item = sword},
                world::InventoryItemPut{.slot = 5, .item = gloves}});
    REQUIRE(view.view.Inventory().size() == 2);
    CHECK(view.view.Inventory()[0].slot == 0);
    CHECK(view.view.Inventory()[0].item == sword);
    CHECK(view.view.Inventory()[1].slot == 5);
    CHECK(view.view.Inventory()[1].item == gloves);
    CHECK(view.LogHas("the inventory came with 2 items"));
    CHECK(view.LogHas("the slot 5 of the inventory has the item of group 10 and number 5, level 0",
                      LogLevel::Debug));

    view.Apply({world::InventoryReplaced{.itemCount = 1}, world::InventoryItemPut{.slot = 5, .item = sword}});
    REQUIRE(view.view.Inventory().size() == 1);
    CHECK(view.view.Inventory()[0].item == sword);
}

TEST_CASE("[0001-CA-09] the changes of the terrain are counted and logged") {
    View view;
    view.Apply({world::TerrainChanged{.start = {10, 20}, .end = {30, 40}, .attributes = 0x01},
                world::TerrainChanged{.start = {1, 2}, .end = {3, 4}, .attributes = 0x04, .removed = true}});
    CHECK(view.view.TerrainChanges() == 2);
    CHECK(view.LogHas("the terrain from (10, 20) to (30, 40) gains the attributes 0x01"));
    CHECK(view.LogHas("the terrain from (1, 2) to (3, 4) loses the attributes 0x04"));
}

TEST_CASE("[0001-CA-09] the events wait in their queue and are applied one by one, in the order they came") {
    View view;
    world::EventQueue queue{8};
    const SessionEpoch epoch{2};
    queue.Push(epoch, Hero());
    queue.Push(epoch, Monster(kMonster, 14, {135, 124}));
    queue.Push(epoch,
               world::ObjectStartedWalking{.id = kMonster, .source = {135, 124}, .target = {137, 125}});
    queue.Push(epoch, world::ObjectLeft{.id = kMonster});
    // Nothing changes before the tick takes the events.
    CHECK_FALSE(view.view.HeroState().loaded);
    CHECK(queue.Drain(
              [&](SessionEpoch /*epoch*/, const world::WorldEvent& event) { view.view.Apply(event); }) == 4);
    CHECK(queue.Empty());
    // In this order the walk finds the monster in view, and it leaves after: nothing is ignored.
    CHECK(view.view.HeroState().loaded);
    CHECK(view.view.Objects().empty());
    CHECK(view.view.IgnoredEvents() == 0);
}

TEST_CASE(
    "[0002-CA-02] the queue keeps each event with the epoch of its message, in order, and keeps its room") {
    world::EventQueue queue{2};
    CHECK(queue.Push(SessionEpoch{1}, world::ObjectLeft{.id = world::ObjectId{0x21}}));
    CHECK(queue.Push(SessionEpoch{2}, world::ObjectLeft{.id = world::ObjectId{0x22}}));
    // Past the room, the queue grows and keeps the event, with its epoch.
    CHECK_FALSE(queue.Push(SessionEpoch{1}, world::ObjectLeft{.id = world::ObjectId{0x23}}));

    struct Taken {
        SessionEpoch epoch;
        world::ObjectId id;
    };
    std::vector<Taken> taken;
    CHECK(queue.Drain([&](SessionEpoch epoch, const world::WorldEvent& event) {
        taken.push_back({.epoch = epoch, .id = std::get<world::ObjectLeft>(event).id});
    }) == 3);
    REQUIRE(taken.size() == 3);
    CHECK(taken[0].epoch == SessionEpoch{1});
    CHECK(taken[0].id == world::ObjectId{0x21});
    CHECK(taken[1].epoch == SessionEpoch{2});
    CHECK(taken[1].id == world::ObjectId{0x22});
    CHECK(taken[2].epoch == SessionEpoch{1});
    CHECK(taken[2].id == world::ObjectId{0x23});

    // The queue is empty, and the room it grew to stays: the next events fit.
    CHECK(queue.Empty());
    CHECK(queue.Push(SessionEpoch{3}, world::ObjectLeft{.id = world::ObjectId{0x24}}));
    CHECK(queue.Push(SessionEpoch{3}, world::ObjectLeft{.id = world::ObjectId{0x25}}));
    CHECK(queue.Push(SessionEpoch{3}, world::ObjectLeft{.id = world::ObjectId{0x26}}));
}

TEST_CASE("[0002-CA-02] a smaller epoch is an older one") {
    CHECK(SessionEpoch{1} < SessionEpoch{2});
    CHECK_FALSE(SessionEpoch{2} < SessionEpoch{2});
    CHECK_FALSE(SessionEpoch{3} < SessionEpoch{2});
    CHECK(SessionEpoch{} < SessionEpoch{1});
}

TEST_CASE("[0001-CA-09] the name of an object keeps at most 10 bytes, up to the first zero") {
    const std::array<std::byte, 12> longName{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'},
                                             std::byte{'e'}, std::byte{'f'}, std::byte{'g'}, std::byte{'h'},
                                             std::byte{'i'}, std::byte{'j'}, std::byte{'k'}, std::byte{'l'}};
    CHECK(world::ObjectName::FromBytes(longName).View() == "abcdefghij");
    const std::array<std::byte, 4> withZero{std::byte{'a'}, std::byte{0}, std::byte{'b'}, std::byte{0}};
    CHECK(world::ObjectName::FromBytes(withZero).View() == "a");
    CHECK(world::ObjectName{}.Empty());
}
