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
}
