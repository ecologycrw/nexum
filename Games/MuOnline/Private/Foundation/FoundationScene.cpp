#include "Mu/Foundation/FoundationScene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mu::foundation {

namespace {

// One step of the square, turning back at the edges of the card.
[[nodiscard]] SquareState Walk(SquareState square) noexcept {
    std::int32_t x = square.x + (square.direction * kSquareStep);
    if (x >= FoundationScene::kMaxX) {
        x = FoundationScene::kMaxX - (x - FoundationScene::kMaxX);
        square.direction = -1;
    } else if (x <= 0) {
        x = -x;
        square.direction = 1;
    }
    square.x = x;
    return square;
}

} // namespace

void FoundationScene::Tick() noexcept {
    m_previous = m_current;
    // The server events come here, before the player commands, and then the commands (M2).
    m_current = Walk(m_current);
    if (m_colorTimer.Tick()) {
        m_current.color = (m_current.color + 1) % kSquareColorCount;
    }
}

FoundationSnapshot FoundationScene::Snapshot(float alpha) const noexcept {
    // Written so that an alpha that is not a number also counts as 0.
    const double weight = alpha > 0.0F ? std::min(static_cast<double>(alpha), 1.0) : 0.0;
    const double x = m_previous.x + ((m_current.x - m_previous.x) * weight);
    return FoundationSnapshot{
        .squareX = static_cast<std::int32_t>(std::lround(x)),
        .squareY = kSquareY,
        .squareColor = m_current.color,
    };
}

} // namespace mu::foundation
