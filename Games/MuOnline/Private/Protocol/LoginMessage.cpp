#include "Mu/Protocol/LoginMessage.hpp"

#include <span>
#include <string>

#include "Mu/Protocol/Cipher.hpp"
#include "Mu/Protocol/Dialect.hpp"

namespace mu::protocol {

namespace {

// The text in a field of its own size, the zeros included, through Xor3 (section 4.4: the whole field).
template <std::size_t Size>
[[nodiscard]] std::array<std::byte, Size> Hidden(std::string_view text) {
    std::array<std::byte, Size> field{};
    static_cast<void>(WriteText(field, text)); // the size was checked before
    Xor3(field);
    return field;
}

[[nodiscard]] std::span<const std::byte> AsBytes(std::string_view text) noexcept {
    return std::as_bytes(std::span<const char>(text.data(), text.size()));
}

} // namespace

nexum::core::Result<void> WriteLogin(std::string_view user, std::string_view password,
                                     std::uint32_t milliseconds, LoginMessage& message) {
    if (user.size() > kLongestUser) {
        return nexum::core::MakeError("the user has " + std::to_string(user.size()) +
                                      " bytes; the most is 10");
    }
    if (password.size() > kLongestPassword) {
        return nexum::core::MakeError("the password has more than 20 bytes; the most is 20");
    }
    auto writer = dialect::Login::Writer::Create(message);
    if (!writer) {
        return tl::unexpected(writer.error());
    }
    if (auto written = writer->SetUsername(Hidden<kLongestUser>(user)); !written) {
        return written;
    }
    if (auto written = writer->SetPassword(Hidden<kLongestPassword>(password)); !written) {
        return written;
    }
    writer->SetTickCount(milliseconds);
    if (auto written = writer->SetClientVersion(AsBytes(kClientIdentity.version)); !written) {
        return written;
    }
    return writer->SetClientSerial(AsBytes(kClientIdentity.serial));
}

} // namespace mu::protocol
