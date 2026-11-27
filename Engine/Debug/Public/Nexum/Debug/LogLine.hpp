#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nexum::debug {

// One line for the log, built in place in a room of fixed size: building it never asks for memory, so the
// code of the hot path can say what it did (INV-13). What does not fit in the room is cut, and Cut() says so.
class LogLine {
public:
    static constexpr std::size_t kRoom = 256;

    LogLine& Append(std::string_view text) noexcept;
    // The value in decimal digits.
    LogLine& AppendNumber(std::uint64_t value) noexcept;
    // The value in hexadecimal, with upper-case letters and at least `digits` digits, such as "0200" for
    // 0x200 with 4 digits. It never writes more than 16 digits.
    LogLine& AppendHex(std::uint64_t value, std::size_t digits) noexcept;

    // The line so far. It stays valid until the line changes.
    [[nodiscard]] std::string_view View() const noexcept { return {m_text.data(), m_size}; }
    [[nodiscard]] bool Cut() const noexcept { return m_cut; }
    void Clear() noexcept;

private:
    std::array<char, kRoom> m_text{};
    std::size_t m_size = 0;
    bool m_cut = false;
};

} // namespace nexum::debug
