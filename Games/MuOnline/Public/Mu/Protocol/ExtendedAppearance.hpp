#pragma once

// The extended appearance of the 106.3 dialect: 27 bytes, which become a neutral appearance. The layout comes
// from the protocol spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, section 7.2)
// and from the finding 2026-11-21-aparencia-estendida.md, which has the examples.

#include <cstddef>
#include <span>

#include "Mu/Characters/Appearance.hpp"

namespace mu::protocol {

inline constexpr std::size_t kExtendedAppearanceSize = 27;

// Reads the appearance at the start of the bytes. A byte past the end reads as 0, and nothing is read past
// the end.
[[nodiscard]] characters::Appearance ReadExtendedAppearance(std::span<const std::byte> bytes) noexcept;

} // namespace mu::protocol
