#include "Nexum/Debug/TerminalLogSink.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <string_view>

namespace nexum::debug {

namespace {

// The length of a view as printf expects it. A longer text is cut: printf cannot take a longer one.
[[nodiscard]] int PrintLength(std::string_view text) noexcept {
    return static_cast<int>(std::min<std::size_t>(text.size(), INT_MAX));
}

// The characters of a view for printf, which must never receive a null pointer, not even for an empty text.
[[nodiscard]] const char* PrintData(std::string_view text) noexcept {
    return text.empty() ? "" : text.data();
}

} // namespace

void TerminalLogSink::Write(const LogRecord& record) noexcept {
    const std::string_view level = ToString(record.level);
    const std::string_view category = record.category.name;
    std::fprintf(stderr, "[%.*s] %.*s: %.*s\n", PrintLength(level), PrintData(level), PrintLength(category),
                 PrintData(category), PrintLength(record.message), PrintData(record.message));
}

} // namespace nexum::debug
