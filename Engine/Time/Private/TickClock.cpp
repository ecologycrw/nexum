#include "Nexum/Time/TickClock.hpp"

#include <algorithm>

namespace nexum::time {

namespace {

// The most frame time one frame may add: kMaxTicksPerFrame ticks, 200 ms (D-05). The time left from the last
// frame is always below one tick, so a frame never has more than kMaxTicksPerFrame ticks to run.
constexpr std::chrono::nanoseconds kMaxFrameTime = kTickDuration * kMaxTicksPerFrame;

} // namespace

FrameTicks TickClock::AdvanceFrame(std::chrono::nanoseconds frameTime) noexcept {
    if (m_paused) {
        return RunRequestedSteps();
    }

    const std::chrono::nanoseconds elapsed = std::max(frameTime, std::chrono::nanoseconds{0});
    const std::chrono::nanoseconds added = std::min(elapsed, kMaxFrameTime);
    m_accumulator += added;

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
