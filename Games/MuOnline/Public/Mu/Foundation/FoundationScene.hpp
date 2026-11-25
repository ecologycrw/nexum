#pragma once

#include <cstdint>

#include "Nexum/Render/TestCard.hpp"
#include "Nexum/Time/TickTimer.hpp"

namespace mu::foundation {

// The numbers of the foundation screen (specs/0000-fundacao/spec.md, "Prova visual"). They belong to the
// slice, not to the original game: the screen exists to show that the ticks keep their pace.
inline constexpr std::int32_t kSquareSide = 32;
// The square walks 4 pixels per tick, 100 pixels per second at 25 ticks per second.
inline constexpr std::int32_t kSquareStep = 4;
// The square takes the next color every 25 ticks: once per second.
inline constexpr std::uint32_t kTicksPerColor = 25;
inline constexpr std::uint32_t kSquareColorCount = 4;

// The square as the simulation knows it after a tick. Its place is counted in pixels from the top-left corner
// of the test card.
struct SquareState {
    std::int32_t x = 0;
    // +1 while the square walks to the right, -1 while it walks to the left.
    std::int32_t direction = 1;
    std::uint32_t color = 0;

    friend bool operator==(const SquareState&, const SquareState&) = default;
};

// What the drawing of one frame reads (INV-04): the square between the last two ticks, in pixels from the
// top-left corner of the test card, and the color of the last tick.
struct FoundationSnapshot {
    std::int32_t squareX = 0;
    std::int32_t squareY = 0;
    std::uint32_t squareColor = 0;

    friend bool operator==(const FoundationSnapshot&, const FoundationSnapshot&) = default;
};

// The simulation of the foundation screen: a square that walks along the free strip at the bottom of the test
// card, from the left edge to the right one and back, and takes the next color every 25 ticks.
class FoundationScene {
public:
    // The farthest place of the square: its right side on the right edge of the card.
    static constexpr std::int32_t kMaxX =
        static_cast<std::int32_t>(nexum::render::kTestCardWidth) - kSquareSide;
    // The square sits in the middle of the free strip of the card.
    static constexpr std::int32_t kSquareY =
        static_cast<std::int32_t>(nexum::render::kTestCardStripTop) +
        ((static_cast<std::int32_t>(nexum::render::kTestCardStripHeight) - kSquareSide) / 2);

    // One tick (III.4 of the reference spec): keeps the state of the previous tick and moves the square.
    // There are no server events nor player commands yet.
    void Tick() noexcept;

    // The square between the previous tick, at alpha 0, and the last one, at alpha 1, rounded to the nearest
    // pixel. An alpha below 0, or not a number, counts as 0, and one above 1 counts as 1.
    [[nodiscard]] FoundationSnapshot Snapshot(float alpha) const noexcept;

    [[nodiscard]] const SquareState& Previous() const noexcept { return m_previous; }

    [[nodiscard]] const SquareState& Current() const noexcept { return m_current; }

private:
    SquareState m_previous;
    SquareState m_current;
    nexum::time::TickTimer m_colorTimer{kTicksPerColor};
};

} // namespace mu::foundation
