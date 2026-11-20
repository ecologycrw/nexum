#pragma once

#include <cstdint>

namespace nexum::time {

// A repeating timer of the simulation, for game logic done every so many ticks. It counts ticks and nothing
// else, so it stops with the simulation and never sees the wall clock (IV.2 of the reference spec). Call
// Tick() once in every tick.
class TickTimer {
public:
    // A timer that fires every `period` ticks, first on the `period`-th tick. A period of 0 never fires.
    explicit TickTimer(std::uint32_t period) noexcept;

    // Counts one tick and says whether the timer fires on it.
    [[nodiscard]] bool Tick() noexcept;

private:
    std::uint32_t m_period;
    std::uint32_t m_elapsed = 0;
};

} // namespace nexum::time
