#include "OtherGame/OtherGame.hpp"

#include "Nexum/Core/Version.hpp"
#include "Nexum/GridRpg/KitIdentity.hpp"

namespace other_game {

std::string DescribeFoundation() {
    return std::string(nexum::gridrpg::KitName()) + " " + nexum::core::ToString(nexum::gridrpg::KitVersion());
}

} // namespace other_game
