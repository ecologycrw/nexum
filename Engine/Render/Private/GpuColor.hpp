#pragma once

#include <string>

#include <SDL3/SDL.h>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"

namespace nexum::render {

// The color SDL GPU expects for a clear: each channel from 0 to 1.
[[nodiscard]] inline SDL_FColor ToFloatColor(core::Rgba8 color) noexcept {
    constexpr float kMaxChannel = 255.0F;
    return SDL_FColor{static_cast<float>(color.r) / kMaxChannel, static_cast<float>(color.g) / kMaxChannel,
                      static_cast<float>(color.b) / kMaxChannel, static_cast<float>(color.a) / kMaxChannel};
}

// An error that ends with SDL's explanation of the last failure.
[[nodiscard]] inline core::Error SdlFailure(const char* what) {
    return core::Error{std::string(what) + ": " + SDL_GetError()};
}

} // namespace nexum::render
