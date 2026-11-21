#include "OtherGame/OtherGame.hpp"

#include "Nexum/Core/Version.hpp"
#include "Nexum/GridRpg/KitIdentity.hpp"

namespace other_game {

std::string DescribeFoundation() {
    return std::string(nexum::gridrpg::KitName()) + " " + nexum::core::ToString(nexum::gridrpg::KitVersion());
}

std::uint32_t Walker::Frame(nexum::time::Seconds frameTime) noexcept {
    const nexum::time::FrameTicks ticks = m_clock.AdvanceFrame(frameTime);
    m_cell += ticks.count;
    return ticks.count;
}

} // namespace other_game
