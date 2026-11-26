#include "Nexum/Platform/Wait.hpp"

#include <SDL3/SDL.h>

namespace nexum::platform {

// SDL_DelayNS, and not the sleep of the standard library: on Windows, the standard sleep counts in steps of
// the system timer, about 15.6 ms by default, and SDL uses a high-resolution timer where the system has one.
void WaitFor(std::chrono::nanoseconds duration) noexcept {
    if (duration <= std::chrono::nanoseconds{0}) {
        return;
    }
    SDL_DelayNS(static_cast<Uint64>(duration.count()));
}

} // namespace nexum::platform
