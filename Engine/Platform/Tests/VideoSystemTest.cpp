#include <doctest/doctest.h>

#include <utility>

#include "Nexum/Platform/VideoSystem.hpp"

using nexum::platform::VideoSystem;

TEST_CASE("the video system starts and names its driver") {
    auto video = VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    MESSAGE("video driver: ", video->DriverName());
    CHECK_FALSE(video->DriverName().empty());
}

TEST_CASE("the video system keeps running while any of its owners is alive") {
    auto first = VideoSystem::Start();
    REQUIRE(first.has_value());
    {
        auto second = VideoSystem::Start();
        REQUIRE(second.has_value());
    }
    CHECK_FALSE(first->DriverName().empty());
}

TEST_CASE("a moved video system hands its ownership over") {
    auto started = VideoSystem::Start();
    REQUIRE(started.has_value());
    VideoSystem video = std::move(*started);
    CHECK(started->DriverName().empty());
    CHECK_FALSE(video.DriverName().empty());
}
