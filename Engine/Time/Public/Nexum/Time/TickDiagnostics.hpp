#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"

namespace nexum::time {

// The upper limits of the bands of the tick time histogram. One more band holds the ticks above 8 ms, 20% of
// the 40 ms budget: if they recur, the simulation may need a thread of its own (D-06 of the reference spec).
inline constexpr std::array<Seconds, 4> kTickTimeBandLimits{
    std::chrono::milliseconds{1}, std::chrono::milliseconds{2}, std::chrono::milliseconds{4},
    std::chrono::milliseconds{8}};
inline constexpr std::size_t kTickTimeBandCount = kTickTimeBandLimits.size() + 1;

// The frame times are sorted into bands of 0.1 ms, up to 200 ms, to find the worst 1% without keeping every
// frame. The last band also holds the frames above 200 ms.
inline constexpr std::chrono::microseconds kFrameTimeBandWidth{100};
inline constexpr std::chrono::milliseconds kFrameTimeBandsEnd{200};
inline constexpr std::size_t kFrameTimeBandCount =
    static_cast<std::size_t>(kFrameTimeBandsEnd / kFrameTimeBandWidth);

// What the tick loop did during a stretch of wall time.
struct TickReport {
    // The wall time the report covers: the sum of its frame times.
    Seconds wallTime{0.0};
    // The ticks run in that time.
    std::uint64_t ticks = 0;
    // The ticks divided by the wall time: 25 while the loop keeps up (D-05).
    double ticksPerSecond = 0.0;
    // The frame time the tick clock dropped, measured in ticks: 50 ms is 1.25 ticks.
    double droppedTicks = 0.0;
    // The frames in that time, and their mean time.
    std::uint64_t frames = 0;
    Seconds meanFrame{0.0};
    // The worst 1% of the frames: the time that 99% of the frames stay within, rounded up to the next
    // 0.1 ms. When it falls in the last band, around or above 200 ms, the longest frame takes its place.
    Seconds worstOnePercentFrame{0.0};
    Seconds longestFrame{0.0};
    // How many ticks took up to 1, 2, 4 and 8 ms to run, and how many took more than 8 ms.
    std::array<std::uint64_t, kTickTimeBandCount> tickTimeBands{};
};

// Gathers the numbers of the tick loop for the log and the metrics overlay (IV.2 and IV.18 of the reference
// spec). Negative, not-a-number and infinite times count as zero.
class TickDiagnostics {
public:
    // Adds one frame: its wall time and what the tick clock did with it.
    void RecordFrame(Seconds frameTime, const FrameTicks& frame) noexcept;

    // Adds the wall time one tick took to run.
    void RecordTickTime(Seconds tickTime) noexcept;

    // The report of everything recorded since the last report. The next report starts from zero.
    [[nodiscard]] TickReport TakeReport() noexcept;

private:
    [[nodiscard]] Seconds WorstOnePercentFrame(std::uint64_t frames, Seconds longestFrame) const noexcept;

    TickReport m_report;
    // How many frames fell into each band of frame time.
    std::array<std::uint64_t, kFrameTimeBandCount> m_frameTimeBands{};
};

} // namespace nexum::time
