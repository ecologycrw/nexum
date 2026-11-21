#include <doctest/doctest.h>

#include <string_view>

#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

using nexum::debug::LogCategory;
using nexum::debug::Logger;
using nexum::debug::LogLevel;
using nexum::debug::ToString;
using nexum::test_support::MemoryLogSink;

namespace {

constexpr LogCategory kRender{"render"};
constexpr LogCategory kNetwork{"network"};

} // namespace

TEST_CASE("[0000-CA-05] the log writes only the messages of its minimum level and above") {
    MemoryLogSink sink;
    Logger logger{sink, LogLevel::Warning};

    logger.Trace(kRender, "trace");
    logger.Debug(kRender, "debug");
    logger.Info(kRender, "info");
    logger.Warning(kRender, "warning");
    logger.Error(kRender, "error");

    REQUIRE(sink.Entries().size() == 2);
    CHECK(sink.Entries()[0].level == LogLevel::Warning);
    CHECK(sink.Entries()[0].message == "warning");
    CHECK(sink.Entries()[1].level == LogLevel::Error);
    CHECK(sink.Entries()[1].message == "error");
    CHECK_FALSE(logger.IsEnabled(LogLevel::Info));
    CHECK(logger.IsEnabled(LogLevel::Error));
}

TEST_CASE("[0000-CA-05] each message carries its category") {
    MemoryLogSink sink;
    Logger logger{sink};
    logger.Info(kRender, "the device is ready");
    logger.Warning(kNetwork, "the server is slow");

    REQUIRE(sink.Entries().size() == 2);
    CHECK(sink.Entries()[0].category == "render");
    CHECK(sink.Entries()[1].category == "network");
}

TEST_CASE("[0000-CA-05] each log writes to the output it was given") {
    MemoryLogSink first;
    MemoryLogSink second;
    Logger toFirst{first};
    Logger toSecond{second};

    toFirst.Info(kRender, "first");
    toSecond.Info(kRender, "second");

    REQUIRE(first.Entries().size() == 1);
    REQUIRE(second.Entries().size() == 1);
    CHECK(first.Entries()[0].message == "first");
    CHECK(second.Entries()[0].message == "second");
}

TEST_CASE("[0000-CA-05] the minimum level can change while the program runs") {
    MemoryLogSink sink;
    Logger logger{sink};
    CHECK(logger.MinimumLevel() == LogLevel::Info);

    logger.Debug(kRender, "hidden");
    logger.SetMinimumLevel(LogLevel::Debug);
    logger.Debug(kRender, "shown");

    REQUIRE(sink.Entries().size() == 1);
    CHECK(sink.Entries()[0].message == "shown");
}

TEST_CASE("the names of the levels are the ones written in the log") {
    CHECK(ToString(LogLevel::Trace) == "trace");
    CHECK(ToString(LogLevel::Debug) == "debug");
    CHECK(ToString(LogLevel::Info) == "info");
    CHECK(ToString(LogLevel::Warning) == "warning");
    CHECK(ToString(LogLevel::Error) == "error");
}
