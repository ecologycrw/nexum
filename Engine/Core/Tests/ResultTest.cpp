#include <doctest/doctest.h>

#include "Nexum/Core/Result.hpp"

using nexum::core::MakeError;
using nexum::core::Result;

namespace {

Result<int> HalfOf(int value) {
    if (value % 2 != 0) {
        return MakeError("the value is odd");
    }
    return value / 2;
}

} // namespace

TEST_CASE("a result carries the value when the operation succeeds") {
    const auto result = HalfOf(8);

    REQUIRE(result.has_value());
    CHECK(*result == 4);
}

TEST_CASE("a result carries the reason when the operation fails") {
    const auto result = HalfOf(7);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message == "the value is odd");
}
