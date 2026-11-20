#include <array>
#include <string_view>

#include <doctest/doctest.h>

#include "Nexum/Core/Version.hpp"

using nexum::core::EngineVersion;
using nexum::core::ParseVersion;
using nexum::core::ToString;
using nexum::core::Version;

TEST_CASE("the engine reports the version it was built with") {
    CHECK(EngineVersion() == Version{0, 0, 0});
}

TEST_CASE("a version is read from its three numbers") {
    const auto version = ParseVersion("1.20.300");

    REQUIRE(version.has_value());
    CHECK(*version == Version{1, 20, 300});
}

TEST_CASE("a version is written as its three numbers") {
    CHECK(ToString(Version{1, 20, 300}) == "1.20.300");
}

TEST_CASE("versions are ordered by major, then minor, then patch") {
    CHECK(Version{1, 0, 0} > Version{0, 9, 9});
    CHECK(Version{1, 2, 0} > Version{1, 1, 9});
    CHECK(Version{1, 1, 2} > Version{1, 1, 1});
}

TEST_CASE("invalid version text is refused with a reason") {
    constexpr std::array<std::string_view, 12> kInvalidTexts{"",       "1",      "1.2",    "1.2.",
                                                             ".1.2",   "1..2",   "a.b.c",  "1.2.3.4",
                                                             "1.2.3 ", " 1.2.3", "-1.2.3", "1.2.99999999999"};

    for (const std::string_view text : kInvalidTexts) {
        CAPTURE(text);
        const auto version = ParseVersion(text);

        REQUIRE_FALSE(version.has_value());
        CHECK_FALSE(version.error().message.empty());
    }
}
