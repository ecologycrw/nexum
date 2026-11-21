#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "Mu/Foundation/TickReportText.hpp"
#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickDiagnostics.hpp"

using mu::foundation::TickReportText;
using nexum::time::Seconds;
using nexum::time::TickReport;

namespace {

// The numbers the line shows; the others stay zero.
TickReport Report(double ticksPerSecond, double droppedTicks, Seconds longestFrame,
                  std::array<std::uint64_t, nexum::time::kTickTimeBandCount> bands = {}) {
    TickReport report;
    report.ticksPerSecond = ticksPerSecond;
    report.droppedTicks = droppedTicks;
    report.longestFrame = longestFrame;
    report.tickTimeBands = bands;
    return report;
}

} // namespace

TEST_CASE("the report of the tick loop is one line, with each number rounded") {
    TickReportText text;
    CHECK(text.Format(Report(24.96, 1.25, Seconds{0.01694}, {120, 3, 1, 0, 1})) ==
          "25.0 ticks per second, 1.25 ticks dropped, longest frame 16.9 ms, "
          "ticks up to 1/2/4/8 ms and above 8 ms: 120/3/1/0/1");
}

TEST_CASE("the digits after the point keep their zeros on the left") {
    TickReportText text;
    CHECK(text.Format(Report(3.04, 0.06, Seconds{0.0002})) ==
          "3.0 ticks per second, 0.06 ticks dropped, longest frame 0.2 ms, "
          "ticks up to 1/2/4/8 ms and above 8 ms: 0/0/0/0/0");
}

TEST_CASE("an empty report says zero everywhere") {
    TickReportText text;
    CHECK(text.Format(TickReport{}) == "0.0 ticks per second, 0.00 ticks dropped, longest frame 0.0 ms, "
                                       "ticks up to 1/2/4/8 ms and above 8 ms: 0/0/0/0/0");
}

TEST_CASE("negative and not-a-number values are written as zero") {
    TickReportText text;
    const std::string_view line =
        text.Format(Report(-3.0, std::numeric_limits<double>::quiet_NaN(), Seconds{-1.0}));
    CHECK(line.starts_with("0.0 ticks per second, 0.00 ticks dropped, longest frame 0.0 ms, "));
}

TEST_CASE("each report replaces the line of the one before") {
    TickReportText text;
    const std::string first(text.Format(Report(25.0, 0.0, Seconds{0.0})));
    const std::string_view second = text.Format(Report(12.5, 0.0, Seconds{0.0}));
    CHECK(first.starts_with("25.0 ticks per second"));
    CHECK(second.starts_with("12.5 ticks per second"));
    CHECK(second.size() == first.size());
}

TEST_CASE("the largest numbers still fit in the line, whole") {
    constexpr std::uint64_t kMost = std::numeric_limits<std::uint64_t>::max();
    TickReportText text;
    const std::string_view line = text.Format(Report(1.0e30, std::numeric_limits<double>::infinity(),
                                                     Seconds{1.0e30}, {kMost, kMost, kMost, kMost, kMost}));
    CHECK(line.starts_with("1000000000000.0 ticks per second, 1000000000000.00 ticks dropped, "
                           "longest frame 1000000000000.0 ms, "));
    CHECK(line.ends_with(": 18446744073709551615/18446744073709551615/18446744073709551615/"
                         "18446744073709551615/18446744073709551615"));
}
