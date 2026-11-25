#include <doctest/doctest.h>

#include <cstdint>
#include <limits>

#include "Mu/Foundation/FoundationScene.hpp"

using mu::foundation::FoundationScene;
using mu::foundation::FoundationSnapshot;
using mu::foundation::SquareState;

namespace {

void RunTicks(FoundationScene& scene, std::uint32_t count) {
    for (std::uint32_t tick = 0; tick < count; ++tick) {
        scene.Tick();
    }
}

} // namespace

TEST_CASE("the square starts at the left edge, in the middle of the free strip of the test card") {
    const FoundationScene scene;
    CHECK(scene.Current() == SquareState{.x = 0, .direction = 1, .color = 0});
    CHECK(scene.Snapshot(0.0F) == FoundationSnapshot{.squareX = 0, .squareY = 424, .squareColor = 0});
    CHECK(FoundationScene::kMaxX == 608);
}

TEST_CASE("each tick keeps the previous state and moves the square 4 pixels") {
    FoundationScene scene;
    scene.Tick();
    CHECK(scene.Previous().x == 0);
    CHECK(scene.Current().x == 4);
    RunTicks(scene, 49);
    CHECK(scene.Previous().x == 196);
    CHECK(scene.Current().x == 200);
}

TEST_CASE("the snapshot of tick 50 with alpha 0.5 puts the square at x 198 with the third color") {
    FoundationScene scene;
    RunTicks(scene, 50);
    CHECK(scene.Snapshot(0.5F) == FoundationSnapshot{.squareX = 198, .squareY = 424, .squareColor = 2});
}

TEST_CASE("the snapshot goes from the previous tick to the last one, rounded to the nearest pixel") {
    FoundationScene scene;
    RunTicks(scene, 10);
    CHECK(scene.Snapshot(0.0F).squareX == 36);
    CHECK(scene.Snapshot(0.1F).squareX == 36);
    CHECK(scene.Snapshot(0.2F).squareX == 37);
    CHECK(scene.Snapshot(0.875F).squareX == 40);
    CHECK(scene.Snapshot(0.999F).squareX == 40);
    CHECK(scene.Snapshot(1.0F).squareX == 40);
}

TEST_CASE("an alpha outside 0 to 1, or not a number, takes the nearest end of the step") {
    FoundationScene scene;
    RunTicks(scene, 10);
    CHECK(scene.Snapshot(-0.5F).squareX == 36);
    CHECK(scene.Snapshot(2.0F).squareX == 40);
    CHECK(scene.Snapshot(std::numeric_limits<float>::quiet_NaN()).squareX == 36);
}

TEST_CASE("the square turns back at the right edge and again at the left edge") {
    FoundationScene scene;
    RunTicks(scene, 152);
    CHECK(scene.Current() == SquareState{.x = 608, .direction = -1, .color = 6 % 4});
    scene.Tick();
    CHECK(scene.Previous().x == 608);
    CHECK(scene.Current().x == 604);
    RunTicks(scene, 151);
    CHECK(scene.Current().x == 0);
    CHECK(scene.Current().direction == 1);
    scene.Tick();
    CHECK(scene.Current().x == 4);
}

TEST_CASE("the square never leaves the card, and the snapshot stays between the edges") {
    FoundationScene scene;
    for (std::uint32_t tick = 0; tick < 1000; ++tick) {
        scene.Tick();
        CAPTURE(tick);
        REQUIRE(scene.Current().x >= 0);
        REQUIRE(scene.Current().x <= FoundationScene::kMaxX);
        const std::int32_t between = scene.Snapshot(0.5F).squareX;
        REQUIRE(between >= 0);
        REQUIRE(between <= FoundationScene::kMaxX);
    }
}

TEST_CASE("the square takes the next color every 25 ticks, and the colors come round again") {
    FoundationScene scene;
    RunTicks(scene, 24);
    CHECK(scene.Current().color == 0);
    scene.Tick();
    CHECK(scene.Current().color == 1);
    CHECK(scene.Previous().color == 0);
    RunTicks(scene, 25);
    CHECK(scene.Current().color == 2);
    RunTicks(scene, 50);
    CHECK(scene.Current().color == 0);
    // The snapshot shows the color of the last tick, without mixing it with the one before.
    CHECK(scene.Snapshot(0.0F).squareColor == 0);
}
