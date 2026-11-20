#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <vector>

#include "Nexum/Time/TickClock.hpp"

using namespace std::chrono_literals;
using nexum::time::FrameTicks;
using nexum::time::TickClock;

namespace {

// One frame of an example in specs/0000-fundacao/plan.md ("Suficiência das descrições") and what it must
// give. The clock keeps whole nanoseconds, so every alpha below is an exact quotient and is compared exactly.
struct ExpectedFrame {
    std::chrono::nanoseconds frameTime{0};
    std::uint32_t ticks = 0;
    std::chrono::nanoseconds droppedTime{0};
    float alpha = 0.0F;
};

void CheckFrames(TickClock& clock, std::initializer_list<ExpectedFrame> frames) {
    int index = 0;
    for (const ExpectedFrame& expected : frames) {
        CAPTURE(index);
        const FrameTicks frame = clock.AdvanceFrame(expected.frameTime);
        CHECK(frame.count == expected.ticks);
        CHECK(frame.droppedTime == expected.droppedTime);
        CHECK(clock.Alpha() == expected.alpha);
        ++index;
    }
}

// Frame times from 5 to 200 ms, many of them with fractions of a millisecond, that add up exactly to `total`.
// The order is irregular but fixed: tests never use the system's random numbers (INV-07).
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

} // namespace

TEST_CASE("[0000-CA-02] short frames add up until a whole tick has passed") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameTime = 16ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.4F},
                           {.frameTime = 16ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.8F},
                           {.frameTime = 16ms, .ticks = 1, .droppedTime = 0ns, .alpha = 0.2F},
                       });
    CHECK(clock.ElapsedTicks() == 1);
}

TEST_CASE("[0000-CA-02] a frame of one second runs 5 ticks and drops the rest") {
    TickClock clock;
    CheckFrames(clock, {{.frameTime = 1000ms, .ticks = 5, .droppedTime = 800ms, .alpha = 0.0F}});
    CHECK(clock.ElapsedTicks() == 5);
}

TEST_CASE("[0000-CA-02] the time left from the last frame still counts after a slow frame") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameTime = 39ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.975F},
                           {.frameTime = 250ms, .ticks = 5, .droppedTime = 50ms, .alpha = 0.975F},
                       });
    CHECK(clock.ElapsedTicks() == 5);
}

TEST_CASE("[0000-CA-01] ten simulated minutes of irregular frames give exactly 15000 ticks") {
    const std::vector<std::chrono::nanoseconds> frames = IrregularFrames(10min);
    const auto [shortest, longest] = std::ranges::minmax(frames);
    REQUIRE(shortest >= 5ms);
    REQUIRE(longest <= 200ms);
    REQUIRE(std::accumulate(frames.begin(), frames.end(), 0ns) == 10min);

    constexpr std::int64_t kUnitsPerTick = 3;
    TickClock clock;
    std::int64_t position = 0;
    std::chrono::nanoseconds droppedTime = 0ns;
    for (const std::chrono::nanoseconds frameTime : frames) {
        const FrameTicks frame = clock.AdvanceFrame(frameTime);
        droppedTime += frame.droppedTime;
        for (std::uint32_t tick = 0; tick < frame.count; ++tick) {
            position += kUnitsPerTick;
        }
    }

    CHECK(clock.ElapsedTicks() == 15'000);
    CHECK(position == 45'000);
    CHECK(droppedTime == 0ns);
    CHECK(clock.Alpha() == 0.0F);
}

TEST_CASE("[0000-CA-02] a paused clock runs no tick, keeps its alpha and throws the paused time away") {
    TickClock clock;
    CHECK(clock.AdvanceFrame(20ms).count == 0);
    clock.Pause();
    REQUIRE(clock.IsPaused());
    for (int frame = 0; frame < 10; ++frame) {
        CAPTURE(frame);
        CheckFrames(clock, {{.frameTime = 100ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.5F}});
    }

    clock.Resume();
    REQUIRE_FALSE(clock.IsPaused());
    CheckFrames(clock, {{.frameTime = 20ms, .ticks = 1, .droppedTime = 0ns, .alpha = 0.0F}});
    CHECK(clock.ElapsedTicks() == 1);
}

TEST_CASE("[0000-CA-02] each step request runs exactly one tick while paused") {
    TickClock clock;
    CHECK(clock.AdvanceFrame(10ms).count == 0);
    clock.Pause();

    clock.RequestStep();
    CheckFrames(clock, {
                           {.frameTime = 16ms, .ticks = 1, .droppedTime = 0ns, .alpha = 0.25F},
                           {.frameTime = 16ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.25F},
                       });

    clock.RequestStep();
    clock.RequestStep();
    CheckFrames(clock, {{.frameTime = 16ms, .ticks = 2, .droppedTime = 0ns, .alpha = 0.25F}});
    CHECK(clock.ElapsedTicks() == 3);
}

TEST_CASE("[0000-CA-02] step requests also run at most 5 ticks per frame") {
    TickClock clock;
    clock.Pause();
    for (int request = 0; request < 7; ++request) {
        clock.RequestStep();
    }
    CHECK(clock.AdvanceFrame(16ms).count == 5);
    CHECK(clock.AdvanceFrame(16ms).count == 2);
    CHECK(clock.AdvanceFrame(16ms).count == 0);
    CHECK(clock.ElapsedTicks() == 7);
}

TEST_CASE("a step request on a running clock changes nothing") {
    TickClock clock;
    clock.RequestStep();
    CHECK(clock.AdvanceFrame(16ms).count == 0);
    clock.Pause();
    CHECK(clock.AdvanceFrame(16ms).count == 0);
    CHECK(clock.ElapsedTicks() == 0);
}

TEST_CASE("resuming forgets the step requests that did not run") {
    TickClock clock;
    clock.Pause();
    clock.RequestStep();
    clock.Resume();
    CHECK(clock.AdvanceFrame(16ms).count == 0);
    CHECK(clock.ElapsedTicks() == 0);
}

TEST_CASE("a negative frame time counts as zero") {
    TickClock clock;
    CheckFrames(clock, {
                           {.frameTime = 20ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.5F},
                           {.frameTime = -30ms, .ticks = 0, .droppedTime = 0ns, .alpha = 0.5F},
                       });
}
