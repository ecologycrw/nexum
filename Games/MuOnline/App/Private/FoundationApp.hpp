#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "Mu/App/ClientOptions.hpp"
#include "Mu/Foundation/FoundationScreen.hpp"
#include "Mu/Online/NetworkTransport.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Session/Session.hpp"
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
#include "Nexum/Time/Seconds.hpp"

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

// The way to the world, when the command line asks for it (specs/0001-rede-ate-o-mundo/spec.md): the
// transport of the real network and the client that uses it. Both are empty without --connect.
struct OnlineParts {
    std::unique_ptr<online::NetworkTransport> transport;
    std::unique_ptr<online::OnlineClient> client;
};

// The client as the platform runs it: the foundation screen in a window (specs/0000-fundacao/spec.md), and
// the session inside its frames when the command line goes to the world.
class FoundationApp final : public nexum::platform::Application {
public:
    // Opens the window, connects it to the GPU, sends the textures and writes to the log the video driver,
    // the GPU backend and the graphics card. With `session`, the client opens the Connect Server as it
    // starts. `log` must outlive the client.
    [[nodiscard]] static nexum::core::Result<std::unique_ptr<nexum::platform::Application>>
    Create(const ClientOptions& options, std::optional<session::SessionOptions> session,
           nexum::debug::Logger& log);

    FoundationApp(FoundationParts parts, OnlineParts online, nexum::debug::Logger& log,
                  std::optional<std::uint32_t> frameLimit);

    [[nodiscard]] nexum::platform::AppStatus Iterate() override;
    [[nodiscard]] nexum::platform::AppStatus OnEvent(const nexum::platform::Event& event) override;

private:
    // Continue while the session runs; the end of the client when it failed or stayed as long as asked.
    [[nodiscard]] nexum::platform::AppStatus CheckSession(nexum::time::Seconds now);
    // Ends the client with this status, with the summary of the session as the last line of the log.
    [[nodiscard]] nexum::platform::AppStatus End(nexum::platform::AppStatus status);
    // Waits until the next tick is due, after a frame the window did not show; `frameStart` is the monotonic
    // time at the start of the frame.
    void WaitForNextTick(nexum::time::Seconds frameStart);

    nexum::debug::Logger* m_log;
    // Before the parts, so the screen, which points to the client, goes first.
    OnlineParts m_online;
    FoundationParts m_parts;
    // The size in pixels of the drawing area of the window, from the WindowResized events.
    nexum::platform::PixelSize m_windowSize;
    std::optional<std::uint32_t> m_frameLimit;
    std::uint32_t m_frames = 0;
    // Started last, so the first frame does not count the time the client took to start.
    nexum::time::MonotonicClock m_clock;
};

} // namespace mu::app
