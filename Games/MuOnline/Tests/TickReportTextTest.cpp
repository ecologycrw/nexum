#include <doctest/doctest.h>

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

TEST_CASE("the report of the tick loop is one line, with each number rounded") {
    TickReportText text;
    const TickReport report{.ticksPerSecond = 24.96,
                            .droppedTicks = 1.25,
                            .frames = 300,
                            .meanFrame = Seconds{0.01666},
                            .worstOnePercentFrame = Seconds{0.0168},
                            .longestFrame = Seconds{0.01694},
                            .tickTimeBands = {120, 3, 1, 0, 1}};
    CHECK(text.Format(report) ==
          "25.0 ticks per second, 1.25 ticks dropped, 300 frames, mean frame 16.7 ms, worst 1% 16.8 ms, "
          "longest frame 16.9 ms, ticks up to 1/2/4/8 ms and above 8 ms: 120/3/1/0/1");
}

TEST_CASE("the digits after the point keep their zeros on the left") {
    TickReportText text;
    const TickReport report{.ticksPerSecond = 3.04, .droppedTicks = 0.06, .longestFrame = Seconds{0.0002}};
    CHECK(text.Format(report) ==
          "3.0 ticks per second, 0.06 ticks dropped, 0 frames, mean frame 0.0 ms, worst 1% 0.0 ms, "
          "longest frame 0.2 ms, ticks up to 1/2/4/8 ms and above 8 ms: 0/0/0/0/0");
}

TEST_CASE("an empty report says zero everywhere") {
    TickReportText text;
    CHECK(text.Format(TickReport{}) ==
          "0.0 ticks per second, 0.00 ticks dropped, 0 frames, mean frame 0.0 ms, worst 1% 0.0 ms, "
          "longest frame 0.0 ms, ticks up to 1/2/4/8 ms and above 8 ms: 0/0/0/0/0");
}

TEST_CASE("negative and not-a-number values are written as zero") {
    constexpr double kNotANumber = std::numeric_limits<double>::quiet_NaN();
    TickReportText text;
    const TickReport report{.ticksPerSecond = -3.0,
                            .droppedTicks = kNotANumber,
                            .meanFrame = Seconds{-1.0},
                            .worstOnePercentFrame = Seconds{kNotANumber},
                            .longestFrame = Seconds{-1.0}};
    CHECK(text.Format(report).starts_with(
        "0.0 ticks per second, 0.00 ticks dropped, 0 frames, mean frame 0.0 ms, "
        "worst 1% 0.0 ms, longest frame 0.0 ms, "));
}

TEST_CASE("each report replaces the line of the one before") {
    TickReportText text;
    const std::string first(text.Format(TickReport{.ticksPerSecond = 25.0}));
    const std::string_view second = text.Format(TickReport{.ticksPerSecond = 12.5});
    CHECK(first.starts_with("25.0 ticks per second"));
    CHECK(second.starts_with("12.5 ticks per second"));
    CHECK(second.size() == first.size());
}

TEST_CASE("the largest numbers still fit in the line, whole") {
    constexpr std::uint64_t kMost = std::numeric_limits<std::uint64_t>::max();
    TickReportText text;
    const TickReport report{.ticksPerSecond = 1.0e30,
                            .droppedTicks = std::numeric_limits<double>::infinity(),
                            .frames = kMost,
                            .meanFrame = Seconds{1.0e30},
                            .worstOnePercentFrame = Seconds{1.0e30},
                            .longestFrame = Seconds{1.0e30},
                            .tickTimeBands = {kMost, kMost, kMost, kMost, kMost}};
    const std::string_view line = text.Format(report);
    CHECK(line.starts_with("1000000000000.0 ticks per second, 1000000000000.00 ticks dropped, "
                           "18446744073709551615 frames, mean frame 1000000000000.0 ms, "
                           "worst 1% 1000000000000.0 ms, longest frame 1000000000000.0 ms, "));
    CHECK(line.ends_with(": 18446744073709551615/18446744073709551615/18446744073709551615/"
                         "18446744073709551615/18446744073709551615"));
}
