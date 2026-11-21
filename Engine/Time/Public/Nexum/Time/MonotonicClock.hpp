#pragma once

#include <cstdint>

#include "Nexum/Time/Seconds.hpp"

namespace nexum::time {

// The monotonic clock of the wall (INV-05): it only moves forward, and a change in the time of day of the
// system does not move it. The frame loop reads the time of each frame from it, and the wall timers run on it
// (D-05 and IV.2 of the reference spec). It reads the standard clock of C++, so Engine/Time needs no other
// module (decision of the owner, specs/0000-fundacao/tasks.md, "Desvios do plano").
class MonotonicClock {
public:
    // A clock that starts at zero now.
    MonotonicClock() noexcept;

    // The time since the clock started.
    [[nodiscard]] Seconds Elapsed() const noexcept;

    // The time since the previous lap, or since the clock started, and the start of a new lap. The frame loop
    // calls it once per frame to get the time of the frame.
    [[nodiscard]] Seconds Lap() noexcept;

private:
    // Readings of the standard clock, in nanoseconds.
    std::int64_t m_start = 0;
    std::int64_t m_lapStart = 0;
};

} // namespace nexum::time
