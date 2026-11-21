#pragma once

#include <atomic>
#include <cstdint>
#include <string_view>

namespace nexum::debug {

// How serious a message is, from the least to the most.
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error };

// The name written in the log: "trace", "debug", "info", "warning" or "error".
[[nodiscard]] std::string_view ToString(LogLevel level) noexcept;

// The part of the program a message comes from, such as "render" or "sdl.gpu". The name must last as long as
// the program, like a string literal: the log keeps only a view of it.
struct LogCategory {
    std::string_view name;
};

// One message, as the log hands it to its output. The views last only during the call to LogSink::Write.
struct LogRecord {
    LogLevel level = LogLevel::Info;
    LogCategory category;
    std::string_view message;
};

// Where the messages of a Logger go: the terminal, or a list in memory in the tests. The log of SDL may call
// it from a thread of its own, so an output used with SdlLogRoute must allow that.
class LogSink {
public:
    LogSink() = default;
    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;
    LogSink(LogSink&&) = delete;
    LogSink& operator=(LogSink&&) = delete;
    virtual ~LogSink() = default;

    virtual void Write(const LogRecord& record) noexcept = 0;
};

// The log of the engine (D-18 of the reference spec). It keeps the messages of its minimum level and above
// and hands them to the output it was given; there is no global log. Writing never asks for memory.
class Logger {
public:
    // A log that writes to `sink`, which must outlive it.
    explicit Logger(LogSink& sink, LogLevel minimumLevel = LogLevel::Info) noexcept;

    // Changes the minimum level; it may be called while other threads write.
    void SetMinimumLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel MinimumLevel() const noexcept;
    [[nodiscard]] bool IsEnabled(LogLevel level) const noexcept;

    void Write(LogLevel level, LogCategory category, std::string_view message) noexcept;

    void Trace(LogCategory category, std::string_view message) noexcept {
        Write(LogLevel::Trace, category, message);
    }

    void Debug(LogCategory category, std::string_view message) noexcept {
        Write(LogLevel::Debug, category, message);
    }

    void Info(LogCategory category, std::string_view message) noexcept {
        Write(LogLevel::Info, category, message);
    }

    void Warning(LogCategory category, std::string_view message) noexcept {
        Write(LogLevel::Warning, category, message);
    }

    void Error(LogCategory category, std::string_view message) noexcept {
        Write(LogLevel::Error, category, message);
    }

private:
    LogSink* m_sink;
    std::atomic<LogLevel> m_minimumLevel;
};

} // namespace nexum::debug
