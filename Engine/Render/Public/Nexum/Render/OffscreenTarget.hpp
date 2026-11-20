#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/GpuDevice.hpp"

namespace nexum::render {

struct TargetSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// An image the GPU draws into without any window, and whose pixels can be read back.
// It is how the visual proof and the parity scenes capture a frame (docs/VISUAL.md).
// The target must be destroyed before its device.
class OffscreenTarget {
public:
    // The largest side Direct3D 12 guarantees for a texture.
    static constexpr std::uint32_t kMaxSide = 16384;

    [[nodiscard]] static core::Result<OffscreenTarget> Create(GpuDevice& device, TargetSize size);

    OffscreenTarget(OffscreenTarget&& other) noexcept;
    OffscreenTarget& operator=(OffscreenTarget&& other) noexcept;
    OffscreenTarget(const OffscreenTarget&) = delete;
    OffscreenTarget& operator=(const OffscreenTarget&) = delete;
    ~OffscreenTarget();

    [[nodiscard]] TargetSize Size() const noexcept;

    // Fills the whole image with one color.
    [[nodiscard]] core::Result<void> Clear(core::Rgba8 color);

    // Waits for the GPU and copies the pixels back, row by row from the top, left to right.
    [[nodiscard]] core::Result<std::vector<core::Rgba8>> ReadPixels();

private:
    struct State;

    explicit OffscreenTarget(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render
