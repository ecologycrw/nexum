#pragma once

#include "Nexum/Core/Version.hpp"

namespace nexum::platform {

// The version of the SDL library in use.
[[nodiscard]] core::Version SdlVersion() noexcept;

} // namespace nexum::platform
