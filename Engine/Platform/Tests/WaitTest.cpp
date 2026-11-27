#include <doctest/doctest.h>

#include <chrono>

#include "Nexum/Platform/Wait.hpp"
#include "Nexum/Time/MonotonicClock.hpp"
#include "Nexum/Time/Seconds.hpp"

using namespace std::chrono_literals;
using nexum::platform::WaitFor;
using nexum::time::MonotonicClock;
using nexum::time::Seconds;

TEST_CASE("[0001-CA-13] a wait lasts at least the time asked") {
    for (const std::chrono::nanoseconds asked : {3ms, 20ms}) {
        CAPTURE(asked.count());
        const MonotonicClock clock;
        WaitFor(asked);
        const Seconds waited = clock.Elapsed();
        MESSAGE("asked ", Seconds{asked}.count() * 1000.0, " ms, waited ", waited.count() * 1000.0, " ms");
        CHECK(waited >= asked);
    }
}

TEST_CASE("a wait of zero or less returns at once") {
    const MonotonicClock clock;
    WaitFor(0ns);
    WaitFor(-1s);
    // Far below the second asked above: the negative wait did not wait at all.
    CHECK(clock.Elapsed() < Seconds{0.5});
}
