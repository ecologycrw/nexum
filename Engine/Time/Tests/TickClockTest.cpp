#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <vector>

#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"

using namespace std::chrono_literals;
using nexum::time::FrameTicks;
using nexum::time::Seconds;
using nexum::time::TickClock;

namespace {

// One frame of an example in specs/0000-fundacao/plan.md ("Suficiência das descrições") and what it must
// give, with the times in seconds as the clock receives them (docs/adr/0019-convencoes.md). The clock adds
// whole nanoseconds, so every alpha below is an exact quotient and is compared exactly.
struct ExpectedFrame {
    double frameSeconds = 0.0;
    std::uint32_t ticks = 0;
    double droppedSeconds = 0.0;
    float alpha = 0.0F;
};

void CheckFrames(TickClock& clock, std::initializer_list<ExpectedFrame> frames) {
    int index = 0;
    for (const ExpectedFrame& expected : frames) {
        CAPTURE(index);
        const FrameTicks frame = clock.AdvanceFrame(Seconds{expected.frameSeconds});
        CHECK(frame.count == expected.ticks);
        CHECK(frame.droppedTime.count() == doctest::Approx(expected.droppedSeconds));
        CHECK(clock.Alpha() == expected.alpha);
        ++index;
    }
}

// Frame times from 5 to 200 ms, almost all with fractions of a millisecond, that add up exactly to `total`.
// They are built in whole nanoseconds so that the total is exact; the test hands them to the clock in
// seconds. The order is irregular but fixed: tests never use the system's random numbers (INV-07).
std::vector<std::chrono::nanoseconds> IrregularFrames(std::chrono::nanoseconds total) {
    constexpr std::chrono::nanoseconds kShortest = 5ms;
    constexpr std::chrono::nanoseconds kLongest = 200ms;
    constexpr std::int64_t kSpread = (kLongest - kShortest).count() + 1;
    constexpr std::int64_t kStride = 37'123'457;

    std::vector<std::chrono::nanoseconds> frames;
    std::chrono::nanoseconds left = total;
    for (std::int64_t index = 0; left > kLongest; ++index) {
        const std::chrono::nanoseconds candidate =
            kShortest + std::chrono::nanoseconds{(index * kStride) % kSpread};
        // Never leave less than the shortest frame for the end.
        frames.push_back(std::min(candidate, left - kShortest));
        left -= frames.back();
    }
    frames.push_back(left);
    return frames;
}

// Whether cutting the time in seconds down to whole nanoseconds, instead of rounding it, would shorten it.
bool CuttingShortens(std::chrono::nanoseconds frameTime) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Seconds{frameTime}) != frameTime;
}

} // namespace

TEST_CASE("[0000-CA-02] short frames add up until a whole tick has passed") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameSeconds = 0.016, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.4F},
                           {.frameSeconds = 0.016, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.8F},
                           {.frameSeconds = 0.016, .ticks = 1, .droppedSeconds = 0.0, .alpha = 0.2F},
                       });
    CHECK(clock.ElapsedTicks() == 1);
}

TEST_CASE("[0000-CA-02] a frame of one second runs 5 ticks and drops the rest") {
    TickClock clock;
    CheckFrames(clock, {{.frameSeconds = 1.0, .ticks = 5, .droppedSeconds = 0.8, .alpha = 0.0F}});
    CHECK(clock.ElapsedTicks() == 5);
}

TEST_CASE("[0000-CA-02] the time left from the last frame still counts after a slow frame") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameSeconds = 0.039, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.975F},
                           {.frameSeconds = 0.25, .ticks = 5, .droppedSeconds = 0.05, .alpha = 0.975F},
                       });
    CHECK(clock.ElapsedTicks() == 5);
}

TEST_CASE("[0000-CA-01] ten simulated minutes of irregular frames give exactly 15000 ticks") {
    const std::vector<std::chrono::nanoseconds> frames = IrregularFrames(10min);
    const auto [shortest, longest] = std::ranges::minmax(frames);
    REQUIRE(shortest >= 5ms);
    REQUIRE(longest <= 200ms);
    REQUIRE(std::accumulate(frames.begin(), frames.end(), 0ns) == 10min);
    // Some frames, such as 0.007817 s, are stored as a double a little below their whole nanoseconds. If the
    // clock cut them instead of rounding, the ten minutes would end one tick short.
    REQUIRE(std::ranges::count_if(frames, CuttingShortens) > 100);

    constexpr std::int64_t kUnitsPerTick = 3;
    TickClock clock;
    std::int64_t position = 0;
    double droppedSeconds = 0.0;
    for (const std::chrono::nanoseconds frameTime : frames) {
        const FrameTicks frame = clock.AdvanceFrame(Seconds{frameTime});
        droppedSeconds += frame.droppedTime.count();
        for (std::uint32_t tick = 0; tick < frame.count; ++tick) {
            position += kUnitsPerTick;
        }
    }

    CHECK(clock.ElapsedTicks() == 15'000);
    CHECK(position == 45'000);
    CHECK(droppedSeconds == 0.0);
    CHECK(clock.Alpha() == 0.0F);
}

TEST_CASE("a time in seconds is rounded to the nearest nanosecond, never cut") {
    // 0.007817 s is stored as a double a little below 7'817'000 ns: cutting it would lose one nanosecond.
    REQUIRE(std::chrono::duration_cast<std::chrono::nanoseconds>(Seconds{0.007817}) == 7'816'999ns);

    TickClock clock;
    CheckFrames(clock, {
                           {.frameSeconds = 0.007817, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.195425F},
                           {.frameSeconds = 0.032183, .ticks = 1, .droppedSeconds = 0.0, .alpha = 0.0F},
                       });
}

TEST_CASE("[0000-CA-02] a paused clock runs no tick, keeps its alpha and throws the paused time away") {
    TickClock clock;
    CHECK(clock.AdvanceFrame(Seconds{0.02}).count == 0);
    clock.Pause();
    REQUIRE(clock.IsPaused());
    for (int frame = 0; frame < 10; ++frame) {
        CAPTURE(frame);
        CheckFrames(clock, {{.frameSeconds = 0.1, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.5F}});
    }

    clock.Resume();
    REQUIRE_FALSE(clock.IsPaused());
    CheckFrames(clock, {{.frameSeconds = 0.02, .ticks = 1, .droppedSeconds = 0.0, .alpha = 0.0F}});
    CHECK(clock.ElapsedTicks() == 1);
}

TEST_CASE("[0000-CA-02] each step request runs exactly one tick while paused") {
    TickClock clock;
    CHECK(clock.AdvanceFrame(Seconds{0.01}).count == 0);
    clock.Pause();

    clock.RequestStep();
    CheckFrames(clock, {
                           {.frameSeconds = 0.016, .ticks = 1, .droppedSeconds = 0.0, .alpha = 0.25F},
                           {.frameSeconds = 0.016, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.25F},
                       });

    clock.RequestStep();
    clock.RequestStep();
    CheckFrames(clock, {{.frameSeconds = 0.016, .ticks = 2, .droppedSeconds = 0.0, .alpha = 0.25F}});
    CHECK(clock.ElapsedTicks() == 3);
}

TEST_CASE("[0000-CA-02] step requests also run at most 5 ticks per frame") {
    TickClock clock;
    clock.Pause();
    for (int request = 0; request < 7; ++request) {
        clock.RequestStep();
    }
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 5);
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 2);
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 0);
    CHECK(clock.ElapsedTicks() == 7);
}

TEST_CASE("a step request on a running clock changes nothing") {
    TickClock clock;
    clock.RequestStep();
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 0);
    clock.Pause();
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 0);
    CHECK(clock.ElapsedTicks() == 0);
}

TEST_CASE("resuming forgets the step requests that did not run") {
    TickClock clock;
    clock.Pause();
    clock.RequestStep();
    clock.Resume();
    CHECK(clock.AdvanceFrame(Seconds{0.016}).count == 0);
    CHECK(clock.ElapsedTicks() == 0);
}

TEST_CASE("a negative, not-a-number or infinite frame time counts as zero") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameSeconds = 0.02, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.5F},
                           {.frameSeconds = -0.03, .ticks = 0, .droppedSeconds = 0.0, .alpha = 0.5F},
                           {.frameSeconds = std::numeric_limits<double>::quiet_NaN(),
                            .ticks = 0,
                            .droppedSeconds = 0.0,
                            .alpha = 0.5F},
                           {.frameSeconds = std::numeric_limits<double>::infinity(),
                            .ticks = 0,
                            .droppedSeconds = 0.0,
                            .alpha = 0.5F},
                       });
    CHECK(clock.ElapsedTicks() == 0);
}
