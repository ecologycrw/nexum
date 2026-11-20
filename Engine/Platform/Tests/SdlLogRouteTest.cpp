#include <doctest/doctest.h>

#include <optional>
#include <string>
#include <vector>

#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/SdlLogRoute.hpp"

using nexum::debug::Logger;
using nexum::debug::LogLevel;
using nexum::debug::LogRecord;
using nexum::debug::LogSink;
using nexum::platform::SdlLogRoute;

namespace {

constexpr const char* kAnnouncement = "the messages of SDL now go to the engine log";

class MemoryLogSink final : public LogSink {
public:
    struct Entry {
        LogLevel level;
        std::string category;
        std::string message;
    };

    void Write(const LogRecord& record) noexcept override {
        m_entries.push_back({record.level, std::string(record.category.name), std::string(record.message)});
    }

    [[nodiscard]] const std::vector<Entry>& Entries() const noexcept { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

} // namespace

TEST_CASE("[0000-CA-05] the messages of SDL go to the engine log while the route exists") {
    MemoryLogSink sink;
    Logger logger{sink, LogLevel::Trace};
    // The route announces itself through the log of SDL, so this message went all the way through SDL.
    const SdlLogRoute route{logger};

    CHECK(route.IsActive());
    REQUIRE(sink.Entries().size() == 1);
    CHECK(sink.Entries()[0].level == LogLevel::Info);
    CHECK(sink.Entries()[0].category == "sdl");
    CHECK(sink.Entries()[0].message == kAnnouncement);
}

TEST_CASE("[0000-CA-05] the newest route gets the messages of SDL and gives them back when it ends") {
    MemoryLogSink outerSink;
    MemoryLogSink innerSink;
    Logger outerLogger{outerSink};
    Logger innerLogger{innerSink};

    const SdlLogRoute outer{outerLogger};
    std::optional<SdlLogRoute> inner;
    inner.emplace(innerLogger);

    CHECK(inner->IsActive());
    CHECK_FALSE(outer.IsActive());
    CHECK(outerSink.Entries().size() == 1);
    CHECK(innerSink.Entries().size() == 1);

    inner.reset();
    CHECK(outer.IsActive());
}
