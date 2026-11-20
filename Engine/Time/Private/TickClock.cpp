#include "Nexum/Time/TickClock.hpp"

#include <algorithm>
#include <cmath>

namespace nexum::time {

namespace {

// The most frame time one frame may add: kMaxTicksPerFrame ticks, 200 ms (D-05). The time left from the last
// frame is always below one tick, so a frame never has more than kMaxTicksPerFrame ticks to run.
constexpr Seconds kMaxFrameTime = kTickDuration * kMaxTicksPerFrame;

// Negative, not-a-number and infinite frame times count as zero (INV-12).
Seconds UsableFrameTime(Seconds frameTime) noexcept {
    const double seconds = frameTime.count();
    return std::isfinite(seconds) && seconds > 0.0 ? frameTime : Seconds{0.0};
}

} // namespace

FrameTicks TickClock::AdvanceFrame(Seconds frameTime) noexcept {
    if (m_paused) {
        return RunRequestedSteps();
    }

    const Seconds elapsed = UsableFrameTime(frameTime);
    const Seconds added = std::min(elapsed, kMaxFrameTime);
    // Rounded, never cut: 0.007817 s, for example, is stored as a double a little below 7'817'000 ns, and
    // cutting would lose one nanosecond on every such frame.
    m_accumulator += std::chrono::round<std::chrono::nanoseconds>(added);

    FrameTicks frame;
    frame.count = static_cast<std::uint32_t>(m_accumulator / kTickDuration);
    frame.droppedTime = elapsed - added;
    m_accumulator -= kTickDuration * frame.count;
    m_elapsedTicks += frame.count;
    return frame;
}

FrameTicks TickClock::RunRequestedSteps() noexcept {
    FrameTicks frame;
    frame.count = std::min(m_requestedSteps, kMaxTicksPerFrame);
    m_requestedSteps -= frame.count;
    m_elapsedTicks += frame.count;
    return frame;
}

float TickClock::Alpha() const noexcept {
    const std::chrono::duration<double, std::nano> accumulator = m_accumulator;
    return static_cast<float>(accumulator / kTickDuration);
}

std::uint64_t TickClock::ElapsedTicks() const noexcept {
    return m_elapsedTicks;
}

void TickClock::Pause() noexcept {
    m_paused = true;
}

void TickClock::Resume() noexcept {
    m_paused = false;
    m_requestedSteps = 0;
}

bool TickClock::IsPaused() const noexcept {
    return m_paused;
}

void TickClock::RequestStep() noexcept {
    if (m_paused) {
        ++m_requestedSteps;
    }
}

} // namespace nexum::time
