#pragma once

// The login of the 106.3 client (LoginLongPassword, C3 F1 01, 60 plain bytes): the user and the password with
// Xor3, the time in ms, the version and the serial of the client. The layout comes from the protocol spec of
// the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 4.4 and 6.6).

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Mu/Protocol/Dialect/Login.hpp"
#include "Nexum/Core/Result.hpp"

namespace mu::protocol {

inline constexpr std::size_t kLoginLength = dialect::Login::kDeclaredLength;

// The longest user and password the packet holds, in bytes of UTF-8 (section 6.6).
inline constexpr std::size_t kLongestUser = 10;
inline constexpr std::size_t kLongestPassword = 20;

using LoginMessage = std::array<std::byte, kLoginLength>;

// Builds the plain login, before Xor32 and SimpleModulus. A user or a password longer than its field is
// refused before anything is written, so it is never cut without a word (the Nexum's own rule).
[[nodiscard]] nexum::core::Result<void> WriteLogin(std::string_view user, std::string_view password,
                                                   std::uint32_t milliseconds, LoginMessage& message);

} // namespace mu::protocol
