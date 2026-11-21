#include "Nexum/Time/WallTimer.hpp"

#include <cmath>
#include <limits>

namespace nexum::time {

namespace {

// The time of a timer that never fires.
constexpr Seconds kNever{std::numeric_limits<double>::infinity()};

[[nodiscard]] bool CanFire(Seconds start, Seconds period) noexcept {
    return std::isfinite(start.count()) && std::isfinite(period.count()) && period.count() > 0.0;
}

} // namespace

WallTimer::WallTimer(Seconds start, Seconds period) noexcept
    : m_period(period), m_next(CanFire(start, period) ? start + period : kNever) {}

bool WallTimer::Fire(Seconds now) noexcept {
    // Compared as plain numbers: the comparisons of std::chrono would let a not-a-number time through.
    if (!std::isfinite(now.count()) || now.count() < m_next.count()) {
        return false;
    }
    const double missedPeriods = std::floor((now - m_next) / m_period);
    m_next += m_period * (missedPeriods + 1.0);
    return true;
}

} // namespace nexum::time
