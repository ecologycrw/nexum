#include "Nexum/Debug/Logger.hpp"

namespace nexum::debug {

std::string_view ToString(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
        return "trace";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Error:
        return "error";
    }
    return "unknown";
}

Logger::Logger(LogSink& sink, LogLevel minimumLevel) noexcept : m_sink(&sink), m_minimumLevel(minimumLevel) {}

void Logger::SetMinimumLevel(LogLevel level) noexcept {
    m_minimumLevel.store(level, std::memory_order_relaxed);
}

LogLevel Logger::MinimumLevel() const noexcept {
    return m_minimumLevel.load(std::memory_order_relaxed);
}

bool Logger::IsEnabled(LogLevel level) const noexcept {
    return level >= MinimumLevel();
}

void Logger::Write(LogLevel level, LogCategory category, std::string_view message) noexcept {
    if (IsEnabled(level)) {
        m_sink->Write(LogRecord{.level = level, .category = category, .message = message});
    }
}

} // namespace nexum::debug
