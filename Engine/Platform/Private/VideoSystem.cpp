#include "Nexum/Platform/VideoSystem.hpp"

#include <string>
#include <utility>

#include <SDL3/SDL.h>

namespace nexum::platform {

core::Result<VideoSystem> VideoSystem::Start() {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        return core::MakeError(std::string("could not start the video system: ") + SDL_GetError());
    }
    VideoSystem video;
    video.m_running = true;
    return video;
}

VideoSystem::VideoSystem(VideoSystem&& other) noexcept : m_running(std::exchange(other.m_running, false)) {}

VideoSystem& VideoSystem::operator=(VideoSystem&& other) noexcept {
    if (this != &other) {
        Stop();
        m_running = std::exchange(other.m_running, false);
    }
    return *this;
}

VideoSystem::~VideoSystem() {
    Stop();
}

std::string_view VideoSystem::DriverName() const noexcept {
    const char* const name = m_running ? SDL_GetCurrentVideoDriver() : nullptr;
    return name != nullptr ? std::string_view(name) : std::string_view();
}

void VideoSystem::Stop() noexcept {
    if (m_running) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        m_running = false;
    }
}

} // namespace nexum::platform
