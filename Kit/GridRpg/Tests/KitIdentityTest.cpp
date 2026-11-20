#include <doctest/doctest.h>

#include "Nexum/Core/Version.hpp"
#include "Nexum/GridRpg/KitIdentity.hpp"

TEST_CASE("the kit carries the version of the engine") {
    CHECK(nexum::gridrpg::KitVersion() == nexum::core::EngineVersion());
}

TEST_CASE("the kit has a name for developers") {
    CHECK(nexum::gridrpg::KitName() == "Nexum GridRpg");
}
