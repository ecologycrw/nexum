#pragma once

#include "Nexum/Time/Seconds.hpp"

namespace nexum::time {

// A repeating timer of the wall clock, for work done every so many seconds, such as the ping or the
// diagnostic report. It fires by the monotonic time alone, independent of frames and ticks, so it keeps
// running while the simulation is paused or a map loads (D-05; I.13.6 of the reference spec).
class WallTimer {
public:
    // A timer that fires every `period`, first at `start + period`, both monotonic times in seconds. A start
    // or a period that is not finite, or a period that is not above zero, never fires.
    WallTimer(Seconds start, Seconds period) noexcept;

    // Whether the timer fires at `now`, the monotonic time of this frame. It fires at most once per call: the
    // periods missed during a long stall become a single firing, and the next one stays on the same rhythm. A
    // time that is not finite never fires.
    [[nodiscard]] bool Fire(Seconds now) noexcept;

private:
    Seconds m_period;
    Seconds m_next;
};

} // namespace nexum::time
