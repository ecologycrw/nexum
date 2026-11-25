#include <doctest/doctest.h>

#include "Nexum/Core/Assert.hpp"

TEST_CASE("[0000-CA-04] NEXUM_ASSERT checks its condition once in the development build and never in the "
          "player build") {
    int evaluations = 0;
    NEXUM_ASSERT(++evaluations == 1);
    CHECK(evaluations == (NEXUM_ASSERTS_ENABLED ? 1 : 0));
}
