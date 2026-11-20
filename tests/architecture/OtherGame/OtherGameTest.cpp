#include <doctest/doctest.h>

#include "OtherGame/OtherGame.hpp"

TEST_CASE("another game of the genre works with the engine and the kit alone") {
    CHECK(other_game::DescribeFoundation() == "Nexum GridRpg 0.0.0");
}
