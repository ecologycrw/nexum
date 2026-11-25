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

OperatingSystem CurrentOperatingSystem() noexcept {
#if defined(SDL_PLATFORM_WINDOWS)
    return OperatingSystem::Windows;
#elif defined(SDL_PLATFORM_MACOS)
    return OperatingSystem::MacOs;
#elif defined(SDL_PLATFORM_LINUX)
    return OperatingSystem::Linux;
#else
#error "The engine runs only on Windows, Linux and macOS (INV-15)."
#endif
}

} // namespace nexum::platform
