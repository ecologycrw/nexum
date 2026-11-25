// A test program of its own, because a false NEXUM_ASSERT ends the whole program in the development build.
// Engine/Core/Tests/CheckAssertFailure.cmake runs it and checks how it ended: in the development build, with
// the message of the assert and no sign that the program went on; in the player build, where the assert is
// off, the program goes on.
#include <doctest/doctest.h>

#include <cstdio>

#include "Nexum/Core/Assert.hpp"

TEST_CASE("[0000-CA-04] a false NEXUM_ASSERT ends the development build with the condition, the file and the "
          "line") {
    const int cells = 2;
    NEXUM_ASSERT(cells + 1 == 4);
    std::puts("the program went on after a false assert");
}
