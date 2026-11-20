#pragma once

#include <cstdint>
#include <string>

namespace mu {

// The development build carries the authoring mode; the player build never does (INV-19).
enum class BuildKind : std::uint8_t { Development, Player };

// The line printed by "MuClient --version": the versions of the client, the engine and the kit,
// and the build kind.
[[nodiscard]] std::string DescribeBuild(BuildKind kind);

} // namespace mu
