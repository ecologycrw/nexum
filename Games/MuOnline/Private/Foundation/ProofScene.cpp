#include "Mu/Foundation/ProofScene.hpp"

#include <cstdint>

#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::foundation {

namespace core = nexum::core;
namespace render = nexum::render;
using nexum::time::Seconds;

namespace {

// 5 ticks of 40 ms, the most one frame runs: 10 of them reach tick 50.
constexpr int kLongFrames = 10;
constexpr Seconds kLongFrame{0.2};
// Half a tick, for alpha 0.5.
constexpr Seconds kHalfTick{0.02};

} // namespace

core::Result<void> RunToTheProofScene(FoundationScreen& screen) {
    Seconds now{0.0};
    for (int frame = 0; frame < kLongFrames; ++frame) {
        now += kLongFrame;
        if (auto framed = screen.Frame(now, kLongFrame, kProofSceneSize); !framed) {
            return framed;
        }
    }
    now += kHalfTick;
    return screen.Frame(now, kHalfTick, kProofSceneSize);
}

core::Result<core::Image> CaptureProofScene(render::GpuDevice& device, nexum::debug::Logger& log) {
    // Declared in this order so they end in the reverse one: the target and the screen before the store.
    auto store =
        render::TextureStore::Create(device, static_cast<std::uint32_t>(FoundationScreen::kTextureCount));
    if (!store) {
        return tl::unexpected(store.error());
    }
    auto screen = FoundationScreen::Create(*store, log, Seconds{0.0});
    if (!screen) {
        return tl::unexpected(screen.error());
    }
    if (auto ran = RunToTheProofScene(*screen); !ran) {
        return tl::unexpected(ran.error());
    }
    auto target = render::OffscreenTarget::Create(device, kProofSceneSize);
    if (!target) {
        return tl::unexpected(target.error());
    }
    if (auto drawn = target->Draw(screen->Drawing(), *store); !drawn) {
        return tl::unexpected(drawn.error());
    }
    return target->ReadImage();
}

} // namespace mu::foundation
