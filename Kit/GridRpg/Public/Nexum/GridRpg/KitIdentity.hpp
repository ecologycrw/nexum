#pragma once

#include <string_view>

#include "Nexum/Core/Version.hpp"

namespace nexum::gridrpg {

// The name of the kit, as shown to developers.
[[nodiscard]] std::string_view KitName() noexcept;

// The kit is released together with the engine, so it carries the version of the engine.
[[nodiscard]] core::Version KitVersion() noexcept;

} // namespace nexum::gridrpg
