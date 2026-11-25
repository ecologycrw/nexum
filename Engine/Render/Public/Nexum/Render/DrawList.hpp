#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/TextureStore.hpp"

namespace nexum::render {

// A copy of a whole texture into the target, with its top-left corner at (x, y), in pixels of the target. The
// parts outside the target are cut off. Every pixel is copied as it is: no scaling, no filter and no
// blending.
struct Blit {
    TextureHandle texture;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// What one frame draws, in order: the whole target filled with the clear color, then the copies. The list
// holds a fixed number of copies and never asks for memory, so a frame can build one every time (INV-13). The
// drawing only reads it (INV-04).
class DrawList {
public:
    static constexpr std::size_t kMaxBlits = 64;

    explicit DrawList(core::Rgba8 clearColor = core::Rgba8{.r = 0, .g = 0, .b = 0, .a = 255}) noexcept
        : m_clearColor(clearColor) {}

    [[nodiscard]] core::Rgba8 ClearColor() const noexcept { return m_clearColor; }

    // Adds a copy after the others. Returns an error, and changes nothing, when the list is full.
    [[nodiscard]] core::Result<void> Add(Blit blit);

    [[nodiscard]] std::span<const Blit> Blits() const noexcept { return std::span(m_blits).first(m_count); }

    // Empties the list for a new frame.
    void Reset(core::Rgba8 clearColor) noexcept {
        m_clearColor = clearColor;
        m_count = 0;
    }

private:
    core::Rgba8 m_clearColor;
    std::array<Blit, kMaxBlits> m_blits{};
    std::size_t m_count = 0;
};

} // namespace nexum::render
