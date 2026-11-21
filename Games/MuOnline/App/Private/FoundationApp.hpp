#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "Mu/App/ClientOptions.hpp"
#include "Mu/Foundation/FoundationScreen.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/Application.hpp"
#include "Nexum/Platform/SdlLogRoute.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Platform/Window.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/Render/WindowTarget.hpp"
#include "Nexum/Time/MonotonicClock.hpp"

namespace mu::app {

// What the client needs to show the foundation screen, in the order it starts. It stops in the reverse order:
// the screen and the window target before the textures and the device, the window before the video system,
// and the route of the SDL messages last.
struct FoundationParts {
    std::unique_ptr<nexum::platform::SdlLogRoute> sdlLog;
    nexum::platform::VideoSystem video;
    nexum::platform::Window window;
    nexum::render::GpuDevice device;
    nexum::render::TextureStore textures;
    nexum::render::WindowTarget target;
    foundation::FoundationScreen screen;
};

// The client as the platform runs it: the foundation screen in a window (specs/0000-fundacao/spec.md).
class FoundationApp final : public nexum::platform::Application {
public:
    // Opens the window, connects it to the GPU, sends the textures and writes to the log the video driver,
    // the GPU backend and the graphics card. `log` must outlive the client.
    [[nodiscard]] static nexum::core::Result<std::unique_ptr<nexum::platform::Application>>
    Create(const ClientOptions& options, nexum::debug::Logger& log);

    FoundationApp(FoundationParts parts, nexum::debug::Logger& log, std::optional<std::uint32_t> frameLimit);

    [[nodiscard]] nexum::platform::AppStatus Iterate() override;
    [[nodiscard]] nexum::platform::AppStatus OnEvent(const nexum::platform::Event& event) override;

private:
    nexum::debug::Logger* m_log;
    FoundationParts m_parts;
    // The size in pixels of the drawing area of the window, from the WindowResized events.
    nexum::platform::PixelSize m_windowSize;
    std::optional<std::uint32_t> m_frameLimit;
    std::uint32_t m_frames = 0;
    // Started last, so the first frame does not count the time the client took to start.
    nexum::time::MonotonicClock m_clock;
};

} // namespace mu::app
