#include "Nexum/Debug/LogLine.hpp"

#include <algorithm>
#include <charconv>

namespace nexum::debug {

namespace {

constexpr std::size_t kMostHexDigits = 16;

} // namespace

LogLine& LogLine::Append(std::string_view text) noexcept {
    const std::size_t room = m_text.size() - m_size;
    const std::size_t count = std::min(text.size(), room);
    std::copy_n(text.begin(), count, m_text.begin() + static_cast<std::ptrdiff_t>(m_size));
    m_size += count;
    m_cut = m_cut || count < text.size();
    return *this;
}

LogLine& LogLine::AppendNumber(std::uint64_t value) noexcept {
    std::array<char, 20> digits{};
    const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), value);
    return Append(std::string_view(digits.data(), static_cast<std::size_t>(written.ptr - digits.data())));
}

LogLine& LogLine::AppendHex(std::uint64_t value, std::size_t digits) noexcept {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::array<char, kMostHexDigits> text{};
    std::size_t count = 0;
    // The digits from the lowest, written from the end of the room to its start.
    do {
        text[kMostHexDigits - 1 - count] = kDigits[value & 0x0FU];
        value >>= 4U;
        ++count;
    } while (value != 0 && count < kMostHexDigits);
    const std::size_t wanted = std::min(digits, kMostHexDigits);
    while (count < wanted) {
        text[kMostHexDigits - 1 - count] = '0';
        ++count;
    }
    return Append(std::string_view(text.data() + (kMostHexDigits - count), count));
}

void LogLine::Clear() noexcept {
    m_size = 0;
    m_cut = false;
}

} // namespace nexum::debug
