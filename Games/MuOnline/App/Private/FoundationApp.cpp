#include "FoundationApp.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "ClientStart.hpp"
#include "Mu/Online/LiveServer.hpp"
#include "Nexum/Debug/Profiler.hpp"
#include "Nexum/Platform/Wait.hpp"
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

[[nodiscard]] OnlineParts MakeOnlineParts(const ClientOptions& options,
                                          std::optional<session::SessionOptions> session,
                                          nexum::debug::Logger& log) {
    OnlineParts parts;
    if (!session) {
        return parts;
    }
    std::optional<nexum::time::Seconds> stay;
    if (options.worldSeconds) {
        stay = nexum::time::Seconds{static_cast<double>(*options.worldSeconds)};
    }
    parts.transport = std::make_unique<online::NetworkTransport>();
    parts.server = std::make_unique<online::LiveServer>(std::move(*session), *parts.transport, log);
    parts.client = std::make_unique<online::OnlineClient>(*parts.server, log, stay);
    return parts;
}

} // namespace

core::Result<std::unique_ptr<platform::Application>>
FoundationApp::Create(const ClientOptions& options, std::optional<session::SessionOptions> session,
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
    return std::unique_ptr<platform::Application>(std::make_unique<FoundationApp>(
        std::move(parts), MakeOnlineParts(options, std::move(session), log), log, options.frameLimit));
}

FoundationApp::FoundationApp(FoundationParts parts, OnlineParts online, nexum::debug::Logger& log,
                             std::optional<std::uint32_t> frameLimit)
    : m_log(&log), m_online(std::move(online)), m_parts(std::move(parts)),
      m_windowSize(m_parts.window.SizeInPixels()), m_frameLimit(frameLimit) {
    if (m_online.client) {
        m_parts.screen.Attach(m_online.client.get());
        m_online.client->Start(m_clock.Elapsed());
    }
}

AppStatus FoundationApp::Iterate() {
    const nexum::time::Seconds frameTime = m_clock.Lap();
    const nexum::time::Seconds now = m_clock.Elapsed();
    const render::TargetSize target{.width = m_windowSize.width, .height = m_windowSize.height};
    if (auto framed = m_parts.screen.Frame(now, frameTime, target); !framed) {
        m_log->Error(kClient, "the frame failed: " + framed.error().message);
        return End(AppStatus::Failure);
    }
    // A window that is not on the screen gets no frame: with Vulkan, asking for its image cost more than the
    // whole frame (T15, on the owner's computer: 8% of one core while minimized, against 0.5% without it).
    const bool seen = m_parts.window.IsVisible();
    if (seen) {
        if (auto shown = m_parts.target.Present(m_parts.screen.Drawing(), m_parts.textures); !shown) {
            m_log->Error(kRender, "could not show the frame: " + shown.error().message);
            return End(AppStatus::Failure);
        }
    }
    NEXUM_PROFILE_FRAME();
    ++m_frames;
    if (const AppStatus session = CheckSession(now); session != AppStatus::Continue) {
        return session;
    }
    if (m_frameLimit && m_frames >= *m_frameLimit) {
        m_log->Info(kClient, "drew " + std::to_string(m_frames) + " frames, as --frames asked");
        return End(AppStatus::Success);
    }
    if (!seen) {
        WaitForNextTick(now);
    }
    return AppStatus::Continue;
}

// A shown frame waits for the screen. A frame that is not shown waits for nothing, and the loop would spin,
// as it did before slice 1: it waits instead until the next tick is due, never longer than one tick, so the
// ticks, the ping and the network keep their pace (specs/0001-rede-ate-o-mundo/plan.md, item 14).
void FoundationApp::WaitForNextTick(nexum::time::Seconds frameStart) {
    platform::WaitFor(m_parts.screen.Ticks().UntilNextTick(m_clock.Elapsed() - frameStart));
}

AppStatus FoundationApp::CheckSession(nexum::time::Seconds now) {
    if (!m_online.client) {
        return AppStatus::Continue;
    }
    switch (m_online.client->Status(now)) {
    case online::OnlineStatus::Running:
        return AppStatus::Continue;
    case online::OnlineStatus::StayedLongEnough:
        m_log->Info(kClient, "stayed in the world as long as --world-seconds asked");
        return End(AppStatus::Success);
    case online::OnlineStatus::ServerEnded:
        m_log->Info(kClient, "the server ended the session");
        return End(AppStatus::Success);
    case online::OnlineStatus::Failed:
        return End(AppStatus::Failure);
    }
    return AppStatus::Continue;
}

AppStatus FoundationApp::End(AppStatus status) {
    if (m_online.client) {
        m_log->Info(kClient, m_online.client->Summary(m_clock.Elapsed()));
    }
    return status;
}

AppStatus FoundationApp::OnEvent(const platform::Event& event) {
    const bool ours = event.window == m_parts.window.Id();
    switch (event.kind) {
    case platform::EventKind::QuitRequested:
        m_log->Info(kClient, "the system asked the client to end");
        return End(AppStatus::Success);
    case platform::EventKind::WindowCloseRequested:
        if (ours) {
            m_log->Info(kClient, "the window was closed");
            return End(AppStatus::Success);
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
