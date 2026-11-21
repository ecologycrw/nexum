#pragma once

#include <cstdint>
#include <string>

#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/TickClock.hpp"

namespace other_game {

// A game written by another developer: it only knows the public interfaces of the engine and of the kit.
[[nodiscard]] std::string DescribeFoundation();

// A piece of that game that walks one cell to the east on every tick of the engine's tick clock.
class Walker {
public:
    // Runs the ticks of one frame and returns how many ran.
    std::uint32_t Frame(nexum::time::Seconds frameTime) noexcept;

    [[nodiscard]] std::int64_t Cell() const noexcept { return m_cell; }
    [[nodiscard]] const nexum::time::TickClock& Clock() const noexcept { return m_clock; }

private:
    nexum::time::TickClock m_clock;
    std::int64_t m_cell = 0;
};

} // namespace other_game
