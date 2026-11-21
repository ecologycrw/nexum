#include <doctest/doctest.h>

#include "Nexum/Time/Seconds.hpp"
#include "OtherGame/OtherGame.hpp"

using nexum::time::Seconds;

TEST_CASE("another game of the genre works with the engine and the kit alone") {
    CHECK(other_game::DescribeFoundation() == "Nexum GridRpg 0.0.0");
}

TEST_CASE("another game of the genre runs on the tick clock of the engine") {
    other_game::Walker walker;

    // One second in frames of 200 ms: 5 ticks each, 25 in all.
    for (int frame = 0; frame < 5; ++frame) {
        CHECK(walker.Frame(Seconds{0.2}) == 5);
    }
    CHECK(walker.Cell() == 25);
    CHECK(walker.Clock().ElapsedTicks() == 25);
    CHECK(walker.Clock().Alpha() == doctest::Approx(0.0));

    // A frame of one second runs at most 5 ticks: the rest is dropped, as for any game of the engine.
    CHECK(walker.Frame(Seconds{1.0}) == 5);
    CHECK(walker.Cell() == 30);
}
