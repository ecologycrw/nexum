#include "Mu/ModuleIdentity.hpp"

#include "Nexum/Core/Version.hpp"
#include "Nexum/GridRpg/KitIdentity.hpp"

namespace mu {

std::string DescribeBuild(BuildKind kind) {
    const std::string engine = nexum::core::ToString(nexum::core::EngineVersion());
    const std::string kit =
        std::string(nexum::gridrpg::KitName()) + " " + nexum::core::ToString(nexum::gridrpg::KitVersion());
    const char* const kindText = kind == BuildKind::Development ? "development build" : "player build";
    // The client is released together with the engine, so it carries the version of the engine.
    return "MuClient " + engine + " (Nexum " + engine + ", " + kit + "), " + kindText;
}

} // namespace mu
