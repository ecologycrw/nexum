#include "Nexum/Time/TickDiagnostics.hpp"

#include <algorithm>
#include <utility>

#include "UsableDuration.hpp"

namespace nexum::time {

namespace {

[[nodiscard]] std::size_t TickTimeBand(Seconds tickTime) noexcept {
    // The ticks above the last limit go to the last band.
    const auto limit = std::ranges::find_if(
        kTickTimeBandLimits, [tickTime](Seconds upperLimit) { return tickTime <= upperLimit; });
    return static_cast<std::size_t>(limit - kTickTimeBandLimits.begin());
}

} // namespace

void TickDiagnostics::RecordFrame(Seconds frameTime, const FrameTicks& frame) noexcept {
    const Seconds usableTime = UsableDuration(frameTime);
    m_report.wallTime += usableTime;
    m_report.ticks += frame.count;
    m_report.droppedTicks += UsableDuration(frame.droppedTime) / kTickDuration;
    m_report.longestFrame = std::max(m_report.longestFrame, usableTime);
}

void TickDiagnostics::RecordTickTime(Seconds tickTime) noexcept {
    ++m_report.tickTimeBands[TickTimeBand(UsableDuration(tickTime))];
}

TickReport TickDiagnostics::TakeReport() noexcept {
    TickReport report = std::exchange(m_report, TickReport{});
    if (report.wallTime.count() > 0.0) {
        report.ticksPerSecond = static_cast<double>(report.ticks) / report.wallTime.count();
    }
    return report;
}

} // namespace nexum::time
