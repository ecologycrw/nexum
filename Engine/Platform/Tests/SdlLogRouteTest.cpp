#include <doctest/doctest.h>

#include <optional>

#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/SdlLogRoute.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

using nexum::debug::Logger;
using nexum::debug::LogLevel;
using nexum::platform::SdlLogRoute;
using nexum::test_support::MemoryLogSink;

namespace {

constexpr const char* kAnnouncement = "the messages of SDL now go to the engine log";

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

TEST_CASE("a route that ends before newer ones leaves them giving the messages back to the route before it") {
    MemoryLogSink sink;
    Logger logger{sink};
    const SdlLogRoute outer{logger};
    std::optional<SdlLogRoute> first;
    std::optional<SdlLogRoute> second;
    std::optional<SdlLogRoute> newest;
    first.emplace(logger);
    second.emplace(logger);
    newest.emplace(logger);

    // The first route ends while two newer ones exist; the newest one keeps the messages.
    first.reset();
    CHECK(newest->IsActive());

    newest.reset();
    CHECK(second->IsActive());

    // The second route gave back the first one; without the hand-over, it would give back a route that ended.
    second.reset();
    CHECK(outer.IsActive());
}
