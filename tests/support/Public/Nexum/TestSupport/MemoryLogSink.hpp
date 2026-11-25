#pragma once

#include <string>
#include <vector>

#include "Nexum/Debug/Logger.hpp"

namespace nexum::test_support {

// A log output that keeps a copy of every message it receives, so a test can look at what reached the log.
// It is meant for one thread: SDL sends its messages to the log on the thread that writes them.
class MemoryLogSink final : public debug::LogSink {
public:
    struct Entry {
        debug::LogLevel level;
        std::string category;
        std::string message;
    };

    void Write(const debug::LogRecord& record) noexcept override {
        m_entries.push_back({record.level, std::string(record.category.name), std::string(record.message)});
    }

    [[nodiscard]] const std::vector<Entry>& Entries() const noexcept { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

} // namespace nexum::test_support
