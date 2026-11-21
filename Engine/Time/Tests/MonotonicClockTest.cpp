#include <doctest/doctest.h>

#include <chrono>
#include <thread>

#include "Nexum/Time/MonotonicClock.hpp"

using nexum::time::MonotonicClock;
using nexum::time::Seconds;

TEST_CASE("[0000-CA-03] the monotonic clock never goes back") {
    const MonotonicClock clock;
    Seconds previous = clock.Elapsed();
    CHECK(previous.count() >= 0.0);
    for (int reading = 0; reading < 10000; ++reading) {
        const Seconds now = clock.Elapsed();
        REQUIRE(now >= previous);
        previous = now;
    }
}

// The standard library promises that a sleep lasts at least the time asked, as a monotonic clock measures it.
// The checks leave one millisecond of room for the conversion to seconds; there is no upper limit, because a
// busy CI machine may wake the test late.
TEST_CASE("[0000-CA-03] the monotonic clock follows the real time") {
    const MonotonicClock clock;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(clock.Elapsed().count() >= 0.019);
}

TEST_CASE("[0000-CA-03] a lap is the time since the previous lap") {
    MonotonicClock clock;
    const Seconds first = clock.Lap();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const Seconds second = clock.Lap();
    const Seconds third = clock.Lap();

    CHECK(first.count() >= 0.0);
    CHECK(second.count() >= 0.009);
    CHECK(third.count() >= 0.0);
    // The laps cover the time of the clock without overlaps. Each lap is rounded to a double on its own, so
    // the sum may pass the time of the clock by a nanosecond at most.
    CHECK((first + second + third).count() <= clock.Elapsed().count() + 1e-9);
}
