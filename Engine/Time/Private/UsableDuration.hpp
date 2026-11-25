#pragma once

#include <cmath>

#include "Nexum/Time/Seconds.hpp"

namespace nexum::time {

// A measured time the module can use: negative, not-a-number and infinite times count as zero (INV-12).
[[nodiscard]] inline Seconds UsableDuration(Seconds duration) noexcept {
    const double seconds = duration.count();
    return std::isfinite(seconds) && seconds > 0.0 ? duration : Seconds{0.0};
}

} // namespace nexum::time
