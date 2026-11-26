#pragma once

// The data of the character (C3 F3 03), which comes with 93 bytes or with 92: the last byte comes only in
// packets of 93 bytes, and fewer than 92 is an error. Source: the protocol spec of the knowledge center,
// docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.11 and 9. The session reads it to enter the world,
// and the translation reads it again for the event of the hero.

#include <array>
#include <cstddef>
#include <span>

#include "Mu/Protocol/Dialect.hpp"
#include "Nexum/Core/Result.hpp"

namespace mu::protocol {

inline constexpr std::size_t kShortestCharacterInformation = 92;

// Room for the whole data, for a packet that lacks the last byte.
using CharacterInformationBytes = std::array<std::byte, dialect::CharacterInformation::kDeclaredLength>;

// Reads the data of the character. A packet of 92 bytes is read from a copy of 93 bytes in `whole`, which
// ends with 0, and the view then points into `whole`, which must outlive it. A packet shorter than 92 bytes
// is an error that says its size.
[[nodiscard]] nexum::core::Result<dialect::CharacterInformation::View>
ReadCharacterInformation(std::span<const std::byte> message, CharacterInformationBytes& whole);

} // namespace mu::protocol
