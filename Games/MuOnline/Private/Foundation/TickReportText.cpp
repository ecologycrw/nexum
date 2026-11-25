#include "Mu/Foundation/TickReportText.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace mu::foundation {

namespace {

// Keep the scaled values far below the largest 64-bit integer.
constexpr double kLargestDecimal = 1.0e12;
constexpr std::uint32_t kMostDecimals = 6;

[[nodiscard]] std::uint64_t PowerOfTen(std::uint32_t exponent) noexcept {
    std::uint64_t power = 1;
    for (std::uint32_t step = 0; step < exponent; ++step) {
        power *= 10;
    }
    return power;
}

} // namespace

std::string_view TickReportText::Format(const nexum::time::TickReport& report) noexcept {
    m_size = 0;
    AppendDecimal(report.ticksPerSecond, 1);
    Append(" ticks per second, ");
    AppendDecimal(report.droppedTicks, 2);
    Append(" ticks dropped, ");
    AppendCount(report.frames);
    Append(" frames, mean frame ");
    AppendDecimal(report.meanFrame.count() * 1000.0, 1);
    Append(" ms, worst 1% ");
    AppendDecimal(report.worstOnePercentFrame.count() * 1000.0, 1);
    Append(" ms, longest frame ");
    AppendDecimal(report.longestFrame.count() * 1000.0, 1);
    Append(" ms, ticks up to 1/2/4/8 ms and above 8 ms: ");
    for (std::size_t band = 0; band < report.tickTimeBands.size(); ++band) {
        if (band > 0) {
            Append("/");
        }
        AppendCount(report.tickTimeBands[band]);
    }
    return {m_text.data(), m_size};
}

void TickReportText::Append(std::string_view text) noexcept {
    const std::size_t room = m_text.size() - m_size;
    const std::size_t count = std::min(text.size(), room);
    std::copy_n(text.begin(), count, m_text.begin() + static_cast<std::ptrdiff_t>(m_size));
    m_size += count;
}

void TickReportText::AppendCount(std::uint64_t value) noexcept {
    std::array<char, 20> digits{};
    const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), value);
    Append(std::string_view(digits.data(), static_cast<std::size_t>(written.ptr - digits.data())));
}

void TickReportText::AppendDecimal(double value, std::uint32_t wantedDecimals) noexcept {
    const std::uint32_t decimals = std::min(wantedDecimals, kMostDecimals);
    const double usable = value > 0.0 ? std::min(value, kLargestDecimal) : 0.0;
    const std::uint64_t unit = PowerOfTen(decimals);
    const auto scaled = static_cast<std::uint64_t>(std::llround(usable * static_cast<double>(unit)));
    AppendCount(scaled / unit);
    if (decimals == 0) {
        return;
    }
    Append(".");
    // The digits after the point, with the zeros on the left that the count would leave out.
    std::uint64_t fraction = scaled % unit;
    std::array<char, 20> digits{};
    for (std::uint32_t position = decimals; position > 0; --position) {
        digits[position - 1] = static_cast<char>('0' + (fraction % 10));
        fraction /= 10;
    }
    Append(std::string_view(digits.data(), decimals));
}

} // namespace mu::foundation
