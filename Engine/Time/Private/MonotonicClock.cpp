#include "Nexum/Time/MonotonicClock.hpp"

#include <chrono>
#include <cstdint>

namespace nexum::time {

namespace {

// steady_clock is the monotonic clock of the standard library: it never goes back.
static_assert(std::chrono::steady_clock::is_steady);

[[nodiscard]] std::int64_t ReadNanoseconds() noexcept {
    const auto sinceStart = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(sinceStart).count();
}

[[nodiscard]] Seconds ToSeconds(std::int64_t nanoseconds) noexcept {
    return std::chrono::duration_cast<Seconds>(std::chrono::nanoseconds{nanoseconds});
}

} // namespace

MonotonicClock::MonotonicClock() noexcept : m_start(ReadNanoseconds()), m_lapStart(m_start) {}

Seconds MonotonicClock::Elapsed() const noexcept {
    return ToSeconds(ReadNanoseconds() - m_start);
}

Seconds MonotonicClock::Lap() noexcept {
    const std::int64_t now = ReadNanoseconds();
    const Seconds lap = ToSeconds(now - m_lapStart);
    m_lapStart = now;
    return lap;
}

} // namespace nexum::time
