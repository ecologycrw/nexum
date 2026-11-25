#include <doctest/doctest.h>

#include <cstdint>
#include <limits>

#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"
#include "Nexum/Time/TickTimer.hpp"
#include "Nexum/Time/WallTimer.hpp"

using nexum::time::FrameTicks;
using nexum::time::Seconds;
using nexum::time::TickClock;
using nexum::time::TickTimer;
using nexum::time::WallTimer;

namespace {

// Runs `frames` frames of `frameSeconds` each and calls the tick timer once per tick. Returns how many times
// it fired.
int RunFrames(TickClock& clock, TickTimer& timer, int frames, double frameSeconds) {
    int fired = 0;
    for (int frame = 0; frame < frames; ++frame) {
        const FrameTicks ticks = clock.AdvanceFrame(Seconds{frameSeconds});
        for (std::uint32_t tick = 0; tick < ticks.count; ++tick) {
            fired += timer.Tick() ? 1 : 0;
        }
    }
    return fired;
}

} // namespace

TEST_CASE("[0000-CA-03] a wall timer fires every period, first one period after its start") {
    WallTimer timer{Seconds{100.0}, Seconds{20.0}};
    CHECK_FALSE(timer.Fire(Seconds{100.0}));
    CHECK_FALSE(timer.Fire(Seconds{119.5}));
    CHECK(timer.Fire(Seconds{120.0}));
    CHECK_FALSE(timer.Fire(Seconds{120.0}));
    CHECK_FALSE(timer.Fire(Seconds{139.75}));
    CHECK(timer.Fire(Seconds{140.25}));
    // The rhythm stays on the multiples of the period, even after a late check.
    CHECK_FALSE(timer.Fire(Seconds{159.75}));
    CHECK(timer.Fire(Seconds{160.0}));
}

TEST_CASE("[0000-CA-03] a wall timer fires once after a long stall and keeps its rhythm") {
    WallTimer timer{Seconds{0.0}, Seconds{20.0}};
    // A map took 55 s to load: the firings at 20, 40 and 60 s become a single one.
    CHECK(timer.Fire(Seconds{75.0}));
    CHECK_FALSE(timer.Fire(Seconds{79.5}));
    CHECK(timer.Fire(Seconds{80.0}));
}

TEST_CASE("[0000-CA-03] a wall timer fires by the monotonic time, also while the simulation is paused") {
    TickClock clock;
    clock.Pause();
    WallTimer timer{Seconds{0.0}, Seconds{5.0}};
    int fired = 0;
    Seconds now{0.0};
    // 10 s in frames of 0.25 s, with no tick at all.
    for (int frame = 0; frame < 40; ++frame) {
        now += Seconds{0.25};
        CHECK(clock.AdvanceFrame(Seconds{0.25}).count == 0);
        fired += timer.Fire(now) ? 1 : 0;
    }
    CHECK(fired == 2);
    CHECK(clock.ElapsedTicks() == 0);
}

TEST_CASE("a wall timer with a period that is not a finite number above zero never fires") {
    for (const double period :
         {0.0, -5.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        CAPTURE(period);
        WallTimer timer{Seconds{0.0}, Seconds{period}};
        CHECK_FALSE(timer.Fire(Seconds{1.0}));
        CHECK_FALSE(timer.Fire(Seconds{1'000'000.0}));
    }
}

TEST_CASE("a wall timer never fires at a time that is not a number or infinite") {
    WallTimer timer{Seconds{0.0}, Seconds{1.0}};
    CHECK_FALSE(timer.Fire(Seconds{std::numeric_limits<double>::quiet_NaN()}));
    CHECK_FALSE(timer.Fire(Seconds{std::numeric_limits<double>::infinity()}));
    CHECK(timer.Fire(Seconds{1.0}));
}

TEST_CASE("[0000-CA-03] a tick timer fires every period ticks") {
    TickTimer timer{3};
    CHECK_FALSE(timer.Tick());
    CHECK_FALSE(timer.Tick());
    CHECK(timer.Tick());
    CHECK_FALSE(timer.Tick());
    CHECK_FALSE(timer.Tick());
    CHECK(timer.Tick());
}

TEST_CASE("[0000-CA-03] a tick timer counts only ticks, never the wall time") {
    // Like the square of the foundation screen, which changes colour every 25 ticks: once a second.
    TickClock clock;
    TickTimer timer{25};

    // 2 s paused: the wall time goes on, but no tick runs, so the timer does not move.
    clock.Pause();
    CHECK(RunFrames(clock, timer, 100, 0.02) == 0);

    // 25 step requests run 25 ticks in 5 frames, only 0.08 s of wall time: the timer fires.
    for (int request = 0; request < 25; ++request) {
        clock.RequestStep();
    }
    CHECK(RunFrames(clock, timer, 5, 0.016) == 1);

    // Running again, 1 s of frames of 20 ms is 25 ticks: the 25th one fires.
    clock.Resume();
    CHECK(RunFrames(clock, timer, 49, 0.02) == 0);
    CHECK(RunFrames(clock, timer, 1, 0.02) == 1);
    CHECK(clock.ElapsedTicks() == 50);
}

TEST_CASE("a tick timer with a period of 0 never fires") {
    TickTimer timer{0};
    for (int tick = 0; tick < 100; ++tick) {
        CHECK_FALSE(timer.Tick());
    }
}
