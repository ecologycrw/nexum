#include "Mu/Protocol/FrameReader.hpp"

#include <algorithm>
#include <string>

namespace mu::protocol {

namespace {

// The largest message: a length in 2 bytes (section 3).
constexpr std::size_t kLargestMessage = 0xFFFF;

[[nodiscard]] bool IsHeaderKind(std::uint8_t first) noexcept {
    return first == static_cast<std::uint8_t>(HeaderKind::C1) ||
           first == static_cast<std::uint8_t>(HeaderKind::C2) ||
           first == static_cast<std::uint8_t>(HeaderKind::C3) ||
           first == static_cast<std::uint8_t>(HeaderKind::C4);
}

// The bytes up to the end of the length field: 2 in C1 and C3, 3 in C2 and C4.
[[nodiscard]] std::size_t LengthFieldEnd(std::byte first) noexcept {
    return HasShortLength(static_cast<HeaderKind>(std::to_integer<std::uint8_t>(first))) ? 2 : 3;
}

[[nodiscard]] nexum::core::Result<void> CheckFirstByte(std::byte first) {
    const auto value = std::to_integer<std::uint8_t>(first);
    if (!IsHeaderKind(value)) {
        return nexum::core::MakeError("the first byte " + HexByte(value) + " is not C1, C2, C3 or C4");
    }
    return {};
}

// The length of the message whose start is in `start`, which holds at least its length field. A length of 0,
// or one that leaves no room for the header and the code, cannot be a message. The spec closes the connection
// on the length 0; the rule for a length shorter than the header and the code is the Nexum's own (INV-12).
[[nodiscard]] nexum::core::Result<std::size_t> MessageLength(std::span<const std::byte> start) {
    const std::size_t length = DeclaredLength(start);
    if (length == 0) {
        return nexum::core::MakeError("a message declares the length 0");
    }
    const auto kind = static_cast<HeaderKind>(std::to_integer<std::uint8_t>(start[0]));
    if (length < HeaderSize(kind) + 1) {
        return nexum::core::MakeError("a message declares " + std::to_string(length) +
                                      " bytes, less than its header and code");
    }
    return length;
}

} // namespace

FrameReader::FrameReader() {
    m_pending.reserve(kLargestMessage);
}

nexum::core::Result<FrameReader::Taken> FrameReader::Take(std::span<const std::byte> piece) {
    if (m_pending.empty()) {
        if (auto first = CheckFirstByte(piece[0]); !first) {
            return tl::unexpected(first.error());
        }
        if (piece.size() >= LengthFieldEnd(piece[0])) {
            auto length = MessageLength(piece);
            if (!length) {
                return tl::unexpected(length.error());
            }
            if (piece.size() >= *length) {
                return Taken{*length, piece.first(*length)}; // the whole message is here: no copy
            }
        }
        m_pending.insert(m_pending.end(), piece.begin(), piece.end());
        return Taken{piece.size(), {}};
    }

    // The start of the message is waiting: complete its length field first, then the rest of it.
    std::size_t used = 0;
    const std::size_t lengthEnd = LengthFieldEnd(m_pending[0]);
    if (m_pending.size() < lengthEnd) {
        used = std::min(lengthEnd - m_pending.size(), piece.size());
        m_pending.insert(m_pending.end(), piece.begin(), piece.begin() + static_cast<std::ptrdiff_t>(used));
        if (m_pending.size() < lengthEnd) {
            return Taken{used, {}};
        }
    }
    auto length = MessageLength(m_pending);
    if (!length) {
        return tl::unexpected(length.error());
    }
    const auto rest = piece.subspan(used);
    const std::size_t missing = std::min(*length - m_pending.size(), rest.size());
    m_pending.insert(m_pending.end(), rest.begin(), rest.begin() + static_cast<std::ptrdiff_t>(missing));
    used += missing;
    if (m_pending.size() == *length) {
        return Taken{used, std::span<const std::byte>(m_pending)};
    }
    return Taken{used, {}};
}

void FrameReader::Forget(std::span<const std::byte> message) noexcept {
    if (message.data() == m_pending.data()) {
        m_pending.clear(); // keeps the room it reserved
    }
}

} // namespace mu::protocol
