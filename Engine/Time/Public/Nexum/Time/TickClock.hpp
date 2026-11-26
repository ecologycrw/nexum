#pragma once

#include <chrono>
#include <cstdint>

#include "Nexum/Time/Seconds.hpp"

namespace nexum::time {

// The length of one simulation tick: the logic runs at 25 ticks per second, the pace of the original client
// (docs/referencia/especificacao-engine-propria-caminho-b.md, I.2 and D-05).
inline constexpr std::chrono::nanoseconds kTickDuration = std::chrono::milliseconds{40};

// The most ticks one frame may run to catch up after a slow frame (D-05).
inline constexpr std::uint32_t kMaxTicksPerFrame = 5;

// What one frame asks the simulation to do.
struct FrameTicks {
    // How many ticks to run now, one after the other, before drawing: from 0 to kMaxTicksPerFrame.
    std::uint32_t count = 0;
    // The part of the frame time above kMaxTicksPerFrame ticks. It never becomes ticks: the simulation time
    // slips, and commands are never repeated to make up for it (D-05).
    Seconds droppedTime{0.0};
};

// The fixed-step clock of the simulation (docs/adr/0005-tick-fixo-de-25-hz.md).
// It reads no system clock: the caller passes the time of each frame, measured by the monotonic clock, so a
// test can simulate minutes in milliseconds.
class TickClock {
public:
    // Adds the time of one frame and says how many ticks to run. The time is rounded to the nearest
    // nanosecond, never cut, and the clock adds whole nanoseconds, so the sums stay exact. A negative,
    // not-a-number or infinite frame time counts as zero.
    [[nodiscard]] FrameTicks AdvanceFrame(Seconds frameTime) noexcept;

    // How far the drawing is from the last tick to the next one: 0 at a tick, always below 1.
    [[nodiscard]] float Alpha() const noexcept;

    // How many ticks the clock has handed out since it was created.
    [[nodiscard]] std::uint64_t ElapsedTicks() const noexcept;

    // The time from now to the next tick, when the last AdvanceFrame was `sinceFrame` ago: never below zero
    // and never above one tick. A frame loop that nobody sees waits this long before its next frame, so it
    // runs one tick per frame and loses none (specs/0001-rede-ate-o-mundo/plan.md, item 14). A paused clock
    // has no tick of its own: it counts one whole tick from its last frame, or nothing while a step request
    // waits. A negative, not-a-number or infinite `sinceFrame` counts as zero.
    [[nodiscard]] std::chrono::nanoseconds UntilNextTick(Seconds sinceFrame) const noexcept;

    // Stops the ticks. The drawing goes on with a fixed alpha, and the paused time is thrown away, so
    // resuming never runs a burst of ticks. This is a decision of the engine, not of the original
    // (specs/0000-fundacao/plan.md).
    void Pause() noexcept;
    // Lets the ticks run again and forgets the step requests that did not run.
    void Resume() noexcept;
    [[nodiscard]] bool IsPaused() const noexcept;

    // While paused, asks for exactly one more tick, run by the next frame that has room for it (at most
    // kMaxTicksPerFrame per frame). A running clock already advances on its own, so a request then changes
    // nothing.
    void RequestStep() noexcept;

private:
    [[nodiscard]] FrameTicks RunRequestedSteps() noexcept;

    // The frame time not yet turned into ticks, in whole nanoseconds: always below one tick.
    std::chrono::nanoseconds m_accumulator{0};
    std::uint64_t m_elapsedTicks = 0;
    std::uint32_t m_requestedSteps = 0;
    bool m_paused = false;
};

} // namespace nexum::time
