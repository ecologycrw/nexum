#include <doctest/doctest.h>

#include "Nexum/Platform/PlatformInfo.hpp"

TEST_CASE("the platform module runs on SDL 3") {
    CHECK(nexum::platform::SdlVersion().major == 3);
}
