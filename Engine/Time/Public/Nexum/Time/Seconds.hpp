#pragma once

#include <chrono>

namespace nexum::time {

// Time of the wall clock in seconds, as a double (docs/adr/0019-convencoes.md). The simulation counts whole
// ticks instead.
using Seconds = std::chrono::duration<double>;

} // namespace nexum::time
