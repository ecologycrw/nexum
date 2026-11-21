#include "Nexum/Time/TickTimer.hpp"

namespace nexum::time {

TickTimer::TickTimer(std::uint32_t period) noexcept : m_period(period) {}

bool TickTimer::Tick() noexcept {
    if (m_period == 0) {
        return false;
    }
    ++m_elapsed;
    if (m_elapsed < m_period) {
        return false;
    }
    m_elapsed = 0;
    return true;
}

} // namespace nexum::time
