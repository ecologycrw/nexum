#include "Nexum/Time/TickDiagnostics.hpp"

#include <algorithm>
#include <utility>

#include "UsableDuration.hpp"

namespace nexum::time {

namespace {

constexpr std::size_t kLastFrameTimeBand = kFrameTimeBandCount - 1;

[[nodiscard]] std::size_t TickTimeBand(Seconds tickTime) noexcept {
    // The ticks above the last limit go to the last band.
    const auto limit = std::ranges::find_if(
        kTickTimeBandLimits, [tickTime](Seconds upperLimit) { return tickTime <= upperLimit; });
    return static_cast<std::size_t>(limit - kTickTimeBandLimits.begin());
}

// Each band holds the frames up to its upper limit, like the bands of the tick time: the first one holds the
// frames up to 0.1 ms, the second one those above 0.1 ms and up to 0.2 ms. The time is rounded to the nearest
// nanosecond first, so a frame of exactly 16.7 ms is not pushed into the next band by the double.
[[nodiscard]] std::size_t FrameTimeBand(Seconds frameTime) noexcept {
    if (frameTime >= kFrameTimeBandsEnd) {
        return kLastFrameTimeBand;
    }
    const auto nanoseconds = std::chrono::round<std::chrono::nanoseconds>(frameTime);
    const std::chrono::nanoseconds width = kFrameTimeBandWidth;
    if (nanoseconds <= width) {
        return 0;
    }
    const auto bandsAbove = (nanoseconds.count() + width.count() - 1) / width.count();
    return std::min(static_cast<std::size_t>(bandsAbove) - 1, kLastFrameTimeBand);
}

} // namespace

void TickDiagnostics::RecordFrame(Seconds frameTime, const FrameTicks& frame) noexcept {
    const Seconds usableTime = UsableDuration(frameTime);
    m_report.wallTime += usableTime;
    m_report.ticks += frame.count;
    m_report.droppedTicks += UsableDuration(frame.droppedTime) / kTickDuration;
    ++m_report.frames;
    m_report.longestFrame = std::max(m_report.longestFrame, usableTime);
    ++m_frameTimeBands[FrameTimeBand(usableTime)];
}

void TickDiagnostics::RecordTickTime(Seconds tickTime) noexcept {
    ++m_report.tickTimeBands[TickTimeBand(UsableDuration(tickTime))];
}

TickReport TickDiagnostics::TakeReport() noexcept {
    TickReport report = std::exchange(m_report, TickReport{});
    if (report.wallTime.count() > 0.0) {
        report.ticksPerSecond = static_cast<double>(report.ticks) / report.wallTime.count();
    }
    if (report.frames > 0) {
        report.meanFrame = report.wallTime / static_cast<double>(report.frames);
        report.worstOnePercentFrame = WorstOnePercentFrame(report.frames, report.longestFrame);
    }
    m_frameTimeBands.fill(0);
    return report;
}

Seconds TickDiagnostics::WorstOnePercentFrame(std::uint64_t frames, Seconds longestFrame) const noexcept {
    // The frame at this position, counted from the shortest, is the one that 99% of the frames stay within.
    // The position is 99% of the frames, rounded up. Dividing by 100 first keeps the product from
    // overflowing.
    const std::uint64_t position = frames / 100 * 99 + ((frames % 100) * 99 + 99) / 100;
    std::uint64_t seen = 0;
    for (std::size_t band = 0; band < kLastFrameTimeBand; ++band) {
        seen += m_frameTimeBands[band];
        if (seen >= position) {
            return kFrameTimeBandWidth * static_cast<double>(band + 1);
        }
    }
    return longestFrame;
}

} // namespace nexum::time
