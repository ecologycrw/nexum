#include "Nexum/Platform/PlatformInfo.hpp"

#include <cstdint>

#include <SDL3/SDL.h>

namespace nexum::platform {

core::Version SdlVersion() noexcept {
    const int version = SDL_GetVersion();
    return core::Version{static_cast<std::uint32_t>(SDL_VERSIONNUM_MAJOR(version)),
                         static_cast<std::uint32_t>(SDL_VERSIONNUM_MINOR(version)),
                         static_cast<std::uint32_t>(SDL_VERSIONNUM_MICRO(version))};
}

} // namespace nexum::platform
