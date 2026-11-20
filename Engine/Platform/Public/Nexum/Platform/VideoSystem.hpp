#pragma once

#include <string_view>

#include "Nexum/Core/Result.hpp"

namespace nexum::platform {

// Keeps the video part of the system running: windows, displays and the GPU drivers.
// Anything that opens a window or a GPU device asks for one, so the order of start and stop stays explicit.
// Several can exist at the same time; the video part stops when the last one is destroyed.
class VideoSystem {
public:
    [[nodiscard]] static core::Result<VideoSystem> Start();

    VideoSystem(VideoSystem&& other) noexcept;
    VideoSystem& operator=(VideoSystem&& other) noexcept;
    VideoSystem(const VideoSystem&) = delete;
    VideoSystem& operator=(const VideoSystem&) = delete;
    ~VideoSystem();

    // The name of the video driver in use, such as "windows", "x11", "wayland" or "cocoa".
    [[nodiscard]] std::string_view DriverName() const noexcept;

private:
    VideoSystem() noexcept = default;
    void Stop() noexcept;

    bool m_running = false;
};

} // namespace nexum::platform
