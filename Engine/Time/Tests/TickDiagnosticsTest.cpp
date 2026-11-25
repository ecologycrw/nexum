#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <limits>

#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"
#include "Nexum/Time/TickDiagnostics.hpp"

using nexum::time::kTickTimeBandCount;
using nexum::time::Seconds;
using nexum::time::TickClock;
using nexum::time::TickDiagnostics;
using nexum::time::TickReport;

namespace {

void RunFrames(TickClock& clock, TickDiagnostics& diagnostics, int frames, double frameSeconds) {
    for (int frame = 0; frame < frames; ++frame) {
        const Seconds frameTime{frameSeconds};
        diagnostics.RecordFrame(frameTime, clock.AdvanceFrame(frameTime));
    }
}

} // namespace

TEST_CASE("[0000-CA-03] the report gives the ticks per second, the dropped ticks and the longest frame") {
    TickClock clock;
    TickDiagnostics diagnostics;
    // 2 s in frames of 50 ms (50 ticks), then a frame of 1 s: 5 ticks, and 800 ms dropped, which is 20 ticks.
    RunFrames(clock, diagnostics, 40, 0.05);
    RunFrames(clock, diagnostics, 1, 1.0);

    const TickReport report = diagnostics.TakeReport();
    CHECK(report.wallTime.count() == doctest::Approx(3.0));
    CHECK(report.ticks == 55);
    CHECK(report.ticksPerSecond == doctest::Approx(55.0 / 3.0));
    CHECK(report.droppedTicks == doctest::Approx(20.0));
    CHECK(report.longestFrame.count() == doctest::Approx(1.0));
    CHECK(report.frames == 41);
    CHECK(report.meanFrame.count() == doctest::Approx(3.0 / 41.0));
}

TEST_CASE("the worst 1% of the frames leaves out only the slowest 1%") {
    TickClock clock;
    TickDiagnostics diagnostics;
    // 100 frames: 99 of 16 ms and one of 50 ms. The 99th frame, from the shortest, is 16 ms.
    RunFrames(clock, diagnostics, 99, 0.016);
    RunFrames(clock, diagnostics, 1, 0.05);
    TickReport report = diagnostics.TakeReport();
    CHECK(report.frames == 100);
    CHECK(report.worstOnePercentFrame.count() == doctest::Approx(0.016));
    CHECK(report.longestFrame.count() == doctest::Approx(0.05));

    // With two slow frames in 100, the second one is inside the 99%.
    RunFrames(clock, diagnostics, 98, 0.016);
    RunFrames(clock, diagnostics, 2, 0.05);
    report = diagnostics.TakeReport();
    CHECK(report.worstOnePercentFrame.count() == doctest::Approx(0.05));
}

TEST_CASE("the worst 1% is rounded up to the next 0.1 ms, and a time on a limit stays on it") {
    TickClock clock;
    TickDiagnostics diagnostics;
    RunFrames(clock, diagnostics, 10, 0.01665);
    CHECK(diagnostics.TakeReport().worstOnePercentFrame.count() == doctest::Approx(0.0167));

    // 16.7 ms as a double is not exactly 16.7 ms; it still counts as up to 16.7 ms.
    RunFrames(clock, diagnostics, 10, 0.0167);
    CHECK(diagnostics.TakeReport().worstOnePercentFrame.count() == doctest::Approx(0.0167));
}

TEST_CASE("a single frame is its own worst 1%") {
    TickClock clock;
    TickDiagnostics diagnostics;
    RunFrames(clock, diagnostics, 1, 0.0333);
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.frames == 1);
    CHECK(report.meanFrame.count() == doctest::Approx(0.0333));
    CHECK(report.worstOnePercentFrame.count() == doctest::Approx(0.0333));
}

TEST_CASE("when the worst 1% is above 200 ms, the report gives the longest frame") {
    TickClock clock;
    TickDiagnostics diagnostics;
    RunFrames(clock, diagnostics, 5, 0.3);
    RunFrames(clock, diagnostics, 1, 0.45);
    CHECK(diagnostics.TakeReport().worstOnePercentFrame.count() == doctest::Approx(0.45));
}

TEST_CASE("the worst 1% of a report counts only the frames after the report before") {
    TickClock clock;
    TickDiagnostics diagnostics;
    RunFrames(clock, diagnostics, 10, 0.1);
    static_cast<void>(diagnostics.TakeReport());

    RunFrames(clock, diagnostics, 10, 0.008);
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.frames == 10);
    CHECK(report.worstOnePercentFrame.count() == doctest::Approx(0.008));
}

TEST_CASE("[0000-CA-03] each report covers only what came after the one before") {
    TickClock clock;
    TickDiagnostics diagnostics;
    RunFrames(clock, diagnostics, 1, 1.0);
    static_cast<void>(diagnostics.TakeReport());

    // 1 s of frames of 40 ms: 25 ticks per second, nothing dropped.
    RunFrames(clock, diagnostics, 25, 0.04);
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.wallTime.count() == doctest::Approx(1.0));
    CHECK(report.ticks == 25);
    CHECK(report.ticksPerSecond == doctest::Approx(25.0));
    CHECK(report.droppedTicks == 0.0);
    CHECK(report.longestFrame.count() == doctest::Approx(0.04));
    CHECK(report.tickTimeBands == std::array<std::uint64_t, kTickTimeBandCount>{});
}

TEST_CASE("[0000-CA-03] a paused simulation reports no ticks per second") {
    TickClock clock;
    TickDiagnostics diagnostics;
    clock.Pause();
    RunFrames(clock, diagnostics, 50, 0.02);
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.wallTime.count() == doctest::Approx(1.0));
    CHECK(report.ticks == 0);
    CHECK(report.ticksPerSecond == 0.0);
}

TEST_CASE("[0000-CA-03] the histogram of the tick time has a band for the ticks above 8 ms") {
    TickDiagnostics diagnostics;
    for (const double seconds : {0.0005, 0.001, 0.0015, 0.003, 0.004, 0.0079, 0.008, 0.0081, 0.012, 0.04}) {
        diagnostics.RecordTickTime(Seconds{seconds});
    }
    // Up to 1 ms, up to 2 ms, up to 4 ms, up to 8 ms, and above 8 ms.
    CHECK(diagnostics.TakeReport().tickTimeBands ==
          std::array<std::uint64_t, kTickTimeBandCount>{2, 1, 2, 2, 3});
}

TEST_CASE("a report with no frame gives zero ticks per second") {
    TickDiagnostics diagnostics;
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.wallTime.count() == 0.0);
    CHECK(report.ticksPerSecond == 0.0);
    CHECK(report.longestFrame.count() == 0.0);
    CHECK(report.frames == 0);
    CHECK(report.meanFrame.count() == 0.0);
    CHECK(report.worstOnePercentFrame.count() == 0.0);
}

TEST_CASE("negative, not-a-number and infinite times count as zero in the report") {
    TickDiagnostics diagnostics;
    for (const double seconds :
         {-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        CAPTURE(seconds);
        diagnostics.RecordFrame(Seconds{seconds}, {});
        diagnostics.RecordTickTime(Seconds{seconds});
    }
    const TickReport report = diagnostics.TakeReport();
    CHECK(report.wallTime.count() == 0.0);
    CHECK(report.longestFrame.count() == 0.0);
    CHECK(report.tickTimeBands == std::array<std::uint64_t, kTickTimeBandCount>{3, 0, 0, 0, 0});
    // They are 3 frames of zero, which fall into the first band: up to 0.1 ms.
    CHECK(report.frames == 3);
    CHECK(report.meanFrame.count() == 0.0);
    CHECK(report.worstOnePercentFrame.count() == doctest::Approx(0.0001));
}
