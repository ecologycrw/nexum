#include "Mu/Foundation/FoundationScreen.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "Mu/Foundation/TickReportText.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Debug/Profiler.hpp"
#include "Nexum/Render/TestCard.hpp"

namespace mu::foundation {

namespace core = nexum::core;
namespace render = nexum::render;
using nexum::time::Seconds;

namespace {

constexpr nexum::debug::LogCategory kTimeCategory{"time"};

// Room for what one frame keeps in the arena: today, only the snapshot.
constexpr std::size_t kArenaCapacity = 4096;

// Where the card starts so that it sits in the middle of the target: below zero when the target is smaller.
[[nodiscard]] std::int32_t CenteredStart(std::uint32_t targetSide, std::uint32_t cardSide) noexcept {
    const std::int64_t start =
        (static_cast<std::int64_t>(targetSide) - static_cast<std::int64_t>(cardSide)) / 2;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(start, std::numeric_limits<std::int32_t>::min(),
                                                              std::numeric_limits<std::int32_t>::max()));
}

// Releases what was already sent; the handles never sent are refused, which changes nothing.
void ReleaseTextures(render::TextureStore& store, const FoundationTextures& textures) noexcept {
    store.Release(textures.card);
    for (const render::TextureHandle square : textures.squares) {
        store.Release(square);
    }
}

[[nodiscard]] core::Result<FoundationTextures> UploadTextures(render::TextureStore& store) {
    FoundationTextures textures;
    auto card = store.Upload(render::MakeTestCard());
    if (!card) {
        return core::MakeError("could not send the test card to the GPU: " + card.error().message);
    }
    textures.card = *card;
    for (std::size_t color = 0; color < kSquareColors.size(); ++color) {
        const auto side = static_cast<std::uint32_t>(kSquareSide);
        auto square = store.Upload(core::Image(side, side, kSquareColors[color]));
        if (!square) {
            ReleaseTextures(store, textures);
            return core::MakeError("could not send the square to the GPU: " + square.error().message);
        }
        textures.squares[color] = *square;
    }
    return textures;
}

} // namespace

core::Result<void> DrawFoundation(const FoundationSnapshot& snapshot, const FoundationTextures& textures,
                                  render::TargetSize target, render::DrawList& list) {
    NEXUM_PROFILE_SCOPE("draw");
    const std::int32_t left = CenteredStart(target.width, render::kTestCardWidth);
    const std::int32_t top = CenteredStart(target.height, render::kTestCardHeight);
    list.Reset(kSurroundColor);
    if (auto card = list.Add(render::Blit{.texture = textures.card, .x = left, .y = top}); !card) {
        return card;
    }
    const render::TextureHandle square = textures.squares[snapshot.squareColor % kSquareColorCount];
    return list.Add(
        render::Blit{.texture = square, .x = left + snapshot.squareX, .y = top + snapshot.squareY});
}

core::Result<FoundationScreen> FoundationScreen::Create(render::TextureStore& store,
                                                        nexum::debug::Logger& log, Seconds start) {
    auto textures = UploadTextures(store);
    if (!textures) {
        return tl::unexpected(textures.error());
    }
    return FoundationScreen(log, *textures, start);
}

FoundationScreen::FoundationScreen(nexum::debug::Logger& log, const FoundationTextures& textures,
                                   Seconds start)
    : m_log(&log), m_textures(textures), m_reportTimer(start, kReportPeriod), m_arena(kArenaCapacity) {}

core::Result<void> FoundationScreen::Frame(Seconds now, Seconds frameTime, render::TargetSize target) {
    // 1. Input: the window events reach the program before the frame; keyboard and mouse come with the slice
    //    that needs them.
    // 2. Network: the translated server events are taken here, without being applied yet.
    if (m_online != nullptr) {
        m_online->TakeNetwork(now);
    }
    // 3. Wall timers: the deadlines and the ping of the session, and the reports.
    if (m_online != nullptr) {
        m_online->AdvanceClock(now);
    }
    ReportTicks(now);
    // 4. Finished loads go to the GPU here, within their time budget (M1).
    // 5. Fixed ticks.
    RunTicks(frameTime);
    // 6. Drawing, which only reads the snapshot.
    return Draw(target);
}

void FoundationScreen::ReportTicks(Seconds now) noexcept {
    if (!m_reportTimer.Fire(now)) {
        return;
    }
    TickReportText text;
    m_log->Info(kTimeCategory, text.Format(m_diagnostics.TakeReport()));
}

void FoundationScreen::RunTicks(Seconds frameTime) {
    const nexum::time::FrameTicks frame = m_ticks.AdvanceFrame(frameTime);
    for (std::uint32_t tick = 0; tick < frame.count; ++tick) {
        NEXUM_PROFILE_SCOPE("tick");
        const Seconds started = m_tickStopwatch.Elapsed();
        // The events of the server come before the systems, so the tick sees the world already corrected.
        if (m_online != nullptr) {
            m_online->ApplyEvents();
        }
        m_scene.Tick();
        m_diagnostics.RecordTickTime(m_tickStopwatch.Elapsed() - started);
    }
    m_diagnostics.RecordFrame(frameTime, frame);
}

core::Result<void> FoundationScreen::Draw(render::TargetSize target) {
    m_arena.Reset();
    auto room = m_arena.AllocateArray<FoundationSnapshot>(1);
    if (!room) {
        return tl::unexpected(room.error());
    }
    FoundationSnapshot& snapshot = room->front();
    snapshot = m_scene.Snapshot(m_ticks.Alpha());
    return DrawFoundation(snapshot, m_textures, target, m_drawing);
}

} // namespace mu::foundation
