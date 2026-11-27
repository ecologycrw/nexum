#pragma once

#include <chrono>

namespace nexum::platform {

// Stops the calling thread for at least `duration`, with the most precise timer of the system, for a frame
// loop that has nothing to do before a given time. The system may wake the thread a little late, never early.
// A zero or negative duration returns at once.
void WaitFor(std::chrono::nanoseconds duration) noexcept;

} // namespace nexum::platform
