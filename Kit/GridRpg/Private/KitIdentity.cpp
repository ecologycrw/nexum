#include "Nexum/GridRpg/KitIdentity.hpp"

namespace nexum::gridrpg {

std::string_view KitName() noexcept {
    return "Nexum GridRpg";
}

core::Version KitVersion() noexcept {
    return core::EngineVersion();
}

} // namespace nexum::gridrpg
