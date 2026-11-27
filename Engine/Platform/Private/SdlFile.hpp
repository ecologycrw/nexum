#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

// What the parts of this module that open files through SDL share. SDL stays inside the module (INV-10).
namespace nexum::platform {

// SDL takes paths in UTF-8, on every system.
[[nodiscard]] inline std::string ToUtf8(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    std::string converted;
    converted.reserve(text.size());
    for (const char8_t unit : text) {
        converted.push_back(static_cast<char>(unit));
    }
    return converted;
}

struct CloseStream {
    void operator()(SDL_IOStream* stream) const noexcept { static_cast<void>(SDL_CloseIO(stream)); }
};

// An open file of SDL, closed when it goes away. A writer that must know whether the last bytes reached the
// disk closes it itself, with SDL_CloseIO, and checks the answer.
using Stream = std::unique_ptr<SDL_IOStream, CloseStream>;

} // namespace nexum::platform
