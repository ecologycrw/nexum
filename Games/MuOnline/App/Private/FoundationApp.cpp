#include "FoundationApp.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "ClientStart.hpp"
#include "Nexum/Debug/Profiler.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TestCard.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::app {

namespace core = nexum::core;
namespace platform = nexum::platform;
namespace render = nexum::render;
using platform::AppStatus;

namespace {

constexpr nexum::debug::LogCategory kClient{"client"};
constexpr nexum::debug::LogCategory kPlatform{"platform"};
constexpr nexum::debug::LogCategory kRender{"render"};

constexpr auto kTextureCapacity = static_cast<std::uint32_t>(foundation::FoundationScreen::kTextureCount);

// The window opens with the size of the test card, in points.
[[nodiscard]] platform::WindowOptions CardWindow() {
    return platform::WindowOptions{
        .title = "MuClient", .width = render::kTestCardWidth, .height = render::kTestCardHeight};
}

} // namespace

core::Result<std::unique_ptr<platform::Application>> FoundationApp::Create(const ClientOptions& options,
                                                                           nexum::debug::Logger& log) {
    auto sdlLog = std::make_unique<platform::SdlLogRoute>(log);
    auto video = StartVideo(log);
    if (!video) {
        return tl::unexpected(video.error());
    }
    auto window = platform::Window::Open(*video, CardWindow());
    if (!window) {
        return tl::unexpected(window.error());
    }
    auto device = CreateDevice(*video, options, log);
    if (!device) {
        return tl::unexpected(device.error());
    }
    auto textures = render::TextureStore::Create(*device, kTextureCapacity);
    if (!textures) {
        return tl::unexpected(textures.error());
    }
    auto target = render::WindowTarget::Claim(*device, *window);
    if (!target) {
        return tl::unexpected(target.error());
    }
    auto screen = foundation::FoundationScreen::Create(*textures, log, nexum::time::Seconds{0.0});
    if (!screen) {
        return tl::unexpected(screen.error());
    }
    FoundationParts parts{std::move(sdlLog),    std::move(*video),  std::move(*window), std::move(*device),
                          std::move(*textures), std::move(*target), std::move(*screen)};
    return std::unique_ptr<platform::Application>(
        std::make_unique<FoundationApp>(std::move(parts), log, options.frameLimit));
}

FoundationApp::FoundationApp(FoundationParts parts, nexum::debug::Logger& log,
                             std::optional<std::uint32_t> frameLimit)
    : m_log(&log), m_parts(std::move(parts)), m_windowSize(m_parts.window.SizeInPixels()),
      m_frameLimit(frameLimit) {}

AppStatus FoundationApp::Iterate() {
    const nexum::time::Seconds frameTime = m_clock.Lap();
    const nexum::time::Seconds now = m_clock.Elapsed();
    const render::TargetSize target{.width = m_windowSize.width, .height = m_windowSize.height};
    if (auto framed = m_parts.screen.Frame(now, frameTime, target); !framed) {
        m_log->Error(kClient, "the frame failed: " + framed.error().message);
        return AppStatus::Failure;
    }
    if (auto shown = m_parts.target.Present(m_parts.screen.Drawing(), m_parts.textures); !shown) {
        m_log->Error(kRender, "could not show the frame: " + shown.error().message);
        return AppStatus::Failure;
    }
    NEXUM_PROFILE_FRAME();
    ++m_frames;
    if (m_frameLimit && m_frames >= *m_frameLimit) {
        m_log->Info(kClient, "drew " + std::to_string(m_frames) + " frames, as --frames asked");
        return AppStatus::Success;
    }
    return AppStatus::Continue;
}

AppStatus FoundationApp::OnEvent(const platform::Event& event) {
    const bool ours = event.window == m_parts.window.Id();
    switch (event.kind) {
    case platform::EventKind::QuitRequested:
        m_log->Info(kClient, "the system asked the client to end");
        return AppStatus::Success;
    case platform::EventKind::WindowCloseRequested:
        if (ours) {
            m_log->Info(kClient, "the window was closed");
            return AppStatus::Success;
        }
        return AppStatus::Continue;
    case platform::EventKind::WindowResized:
        if (ours) {
            m_windowSize = event.size;
        }
        return AppStatus::Continue;
    case platform::EventKind::WindowMinimized:
        m_log->Info(kPlatform, "the window was minimized");
        return AppStatus::Continue;
    case platform::EventKind::WindowRestored:
        m_log->Info(kPlatform, "the window was restored");
        return AppStatus::Continue;
    }
    return AppStatus::Continue;
}

} // namespace mu::app
