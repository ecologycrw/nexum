#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Nexum/Time/TickDiagnostics.hpp"

namespace mu::foundation {

// The report of the tick loop as one line of the log, such as
// "25.0 ticks per second, 0.00 ticks dropped, 300 frames, mean frame 16.7 ms, worst 1% 16.8 ms, longest frame
// 16.9 ms, ticks up to 1/2/4/8 ms and above 8 ms: 125/0/0/0/0". The line is built in place, so writing it
// never asks for memory (INV-13).
class TickReportText {
public:
    // The line of `report`. It stays valid until the next call.
    [[nodiscard]] std::string_view Format(const nexum::time::TickReport& report) noexcept;

private:
    void Append(std::string_view text) noexcept;
    void AppendCount(std::uint64_t value) noexcept;
    // `value` rounded to `wantedDecimals` digits after the point, at most 6. Negative and not-a-number values
    // count as zero.
    void AppendDecimal(double value, std::uint32_t wantedDecimals) noexcept;

    // Enough for the longest report; anything longer would be cut.
    std::array<char, 384> m_text{};
    std::size_t m_size = 0;
};

} // namespace mu::foundation
