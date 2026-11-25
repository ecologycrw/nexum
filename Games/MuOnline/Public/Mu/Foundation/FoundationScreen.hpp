#pragma once

#include <array>
#include <cstddef>

#include "Mu/Foundation/FoundationScene.hpp"
#include "Nexum/Core/FrameArena.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/Time/MonotonicClock.hpp"
#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"
#include "Nexum/Time/TickDiagnostics.hpp"
#include "Nexum/Time/WallTimer.hpp"

namespace mu::foundation {

// The colors the square takes, one after the other.
inline constexpr std::array<nexum::core::Rgba8, kSquareColorCount> kSquareColors{
    nexum::core::Rgba8{.r = 255, .g = 128, .b = 0, .a = 255},
    nexum::core::Rgba8{.r = 0, .g = 200, .b = 120, .a = 255},
    nexum::core::Rgba8{.r = 255, .g = 64, .b = 160, .a = 255},
    nexum::core::Rgba8{.r = 96, .g = 160, .b = 255, .a = 255},
};

// The color around the test card when the target is larger than the card.
inline constexpr nexum::core::Rgba8 kSurroundColor{.r = 16, .g = 16, .b = 16, .a = 255};

// The textures the foundation screen draws with: the test card and one square of each color.
struct FoundationTextures {
    nexum::render::TextureHandle card;
    std::array<nexum::render::TextureHandle, kSquareColorCount> squares{};
};

// The drawing of one frame (INV-04): it only reads the snapshot. The test card goes to the middle of the
// target, with the surround color around it, and the square over the card. A target smaller than the card
// cuts its edges.
[[nodiscard]] nexum::core::Result<void> DrawFoundation(const FoundationSnapshot& snapshot,
                                                       const FoundationTextures& textures,
                                                       nexum::render::TargetSize target,
                                                       nexum::render::DrawList& list);

// The frames of the foundation screen, in the order of III.4 of the reference spec: the wall timers, the
// ticks, the snapshot in the frame arena, and then the drawing. The caller shows the draw list in a window or
// draws it without one. A frame never asks for memory (INV-13).
class FoundationScreen {
public:
    // The wall time between two reports of the tick loop in the log.
    static constexpr nexum::time::Seconds kReportPeriod{5.0};
    // The textures the screen keeps in its store.
    static constexpr std::size_t kTextureCount = 1 + kSquareColorCount;

    // Sends the test card and the squares to the GPU. The store must outlive the screen, and so must `log`;
    // `start` is the monotonic time of the screen's first frame, for the reports. The textures stay in the
    // store until it is destroyed.
    [[nodiscard]] static nexum::core::Result<FoundationScreen>
    Create(nexum::render::TextureStore& store, nexum::debug::Logger& log, nexum::time::Seconds start);

    // One frame. `now` is the monotonic time of the frame and `frameTime` the time since the previous one;
    // `target` is the size in pixels of what the frame will be drawn into.
    [[nodiscard]] nexum::core::Result<void> Frame(nexum::time::Seconds now, nexum::time::Seconds frameTime,
                                                  nexum::render::TargetSize target);

    // What the last frame draws, for WindowTarget::Present or OffscreenTarget::Draw.
    [[nodiscard]] const nexum::render::DrawList& Drawing() const noexcept { return m_drawing; }

    [[nodiscard]] const nexum::time::TickClock& Ticks() const noexcept { return m_ticks; }

    [[nodiscard]] const FoundationScene& Scene() const noexcept { return m_scene; }

private:
    FoundationScreen(nexum::debug::Logger& log, const FoundationTextures& textures,
                     nexum::time::Seconds start);

    void ReportTicks(nexum::time::Seconds now) noexcept;
    void RunTicks(nexum::time::Seconds frameTime) noexcept;
    [[nodiscard]] nexum::core::Result<void> Draw(nexum::render::TargetSize target);

    nexum::debug::Logger* m_log;
    FoundationTextures m_textures;
    nexum::time::TickClock m_ticks;
    nexum::time::TickDiagnostics m_diagnostics;
    nexum::time::WallTimer m_reportTimer;
    // Measures how long each tick takes, for the histogram of the reports.
    nexum::time::MonotonicClock m_tickStopwatch;
    FoundationScene m_scene;
    nexum::core::FrameArena m_arena;
    nexum::render::DrawList m_drawing;
};

} // namespace mu::foundation
