#pragma once

#include "Mu/Foundation/FoundationScreen.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TestCard.hpp"

namespace mu::foundation {

// The proof scene of slice 0 (specs/0000-fundacao/spec.md, "Prova visual"): the test card at its own size,
// 640x480 pixels, at tick 50 with alpha 0.5. There is no camera: everything is drawn in pixels.
inline constexpr nexum::render::TargetSize kProofSceneSize{.width = nexum::render::kTestCardWidth,
                                                           .height = nexum::render::kTestCardHeight};

// Brings a new screen to the proof scene: 10 frames of 200 ms run 5 ticks each, and 20 ms more is half a
// tick.
[[nodiscard]] nexum::core::Result<void> RunToTheProofScene(FoundationScreen& screen);

// Draws the proof scene on the GPU without a window and reads it back. It is what MuClient --capture writes
// and the visual proof compares with the approved image (CA-07). The screen writes its messages to `log`.
[[nodiscard]] nexum::core::Result<nexum::core::Image> CaptureProofScene(nexum::render::GpuDevice& device,
                                                                        nexum::debug::Logger& log);

} // namespace mu::foundation
