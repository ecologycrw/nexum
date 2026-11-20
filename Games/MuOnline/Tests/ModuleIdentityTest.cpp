#include <string>

#include <doctest/doctest.h>

#include "Mu/ModuleIdentity.hpp"

using mu::BuildKind;
using mu::DescribeBuild;

TEST_CASE("the description names the client, the engine and the kit with their versions") {
    CHECK(DescribeBuild(BuildKind::Development)
              .starts_with("MuClient 0.0.0 (Nexum 0.0.0, Nexum GridRpg 0.0.0), "));
}

TEST_CASE("the development build says so in its description") {
    CHECK(DescribeBuild(BuildKind::Development).ends_with("development build"));
}

TEST_CASE("the player build says so in its description") {
    CHECK(DescribeBuild(BuildKind::Player).ends_with("player build"));
}
