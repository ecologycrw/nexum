#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "AllocationCounter.hpp"
#include "Mu/Foundation/FoundationScreen.hpp"
#include "Mu/Foundation/ProofScene.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/Time/Seconds.hpp"

using frame_allocations::AllocationCounter;
using frame_allocations::AllocationTally;
using mu::foundation::FoundationScreen;
using nexum::time::Seconds;

namespace {

constexpr int kWarmUpFrames = 3;
constexpr int kMeasuredFrames = 1000;

// Frame times in seconds, repeated in this order: frames with no tick, with one tick and with several, and
// one of 250 ms whose time above 5 ticks is dropped. 1000 frames are about 60 seconds, with a report of the
// tick loop in the log every 5 seconds.
constexpr std::array<double, 8> kFrameTimes{0.016, 0.017, 0.033, 0.008, 0.041, 0.1, 0.25, 0.0167};

// C++ lets the compiler remove a request for memory whose memory is never used, and Clang does it in the
// optimized builds. Writing the address to a volatile object uses it, so the requests the test makes on
// purpose really happen.
const void* volatile gKeptAddress = nullptr;

void KeepAddress(const void* address) noexcept {
    gKeptAddress = address;
}

// A log output that keeps nothing but the number of messages, so that writing to the log asks for no memory.
class CountingLogSink final : public nexum::debug::LogSink {
public:
    void Write(const nexum::debug::LogRecord& /*record*/) noexcept override { ++m_messages; }

    [[nodiscard]] std::uint64_t Messages() const noexcept { return m_messages; }

private:
    std::uint64_t m_messages = 0;
};

// What the test needs, in the order the engine requires: video first, then the device and its store. The
// tests of the screen (FoundationScreenTest.cpp) have the first copy of this; this is the second.
struct Gpu {
    nexum::platform::VideoSystem video;
    nexum::render::GpuDevice device;
    nexum::render::TextureStore store;
};

Gpu StartGpu() {
    auto video = nexum::platform::VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    auto device = nexum::render::GpuDevice::Create(*video, nexum::render::GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    auto store = nexum::render::TextureStore::Create(
        *device, static_cast<std::uint32_t>(FoundationScreen::kTextureCount));
    REQUIRE_MESSAGE(store.has_value(), (store ? "" : store.error().message));
    return Gpu{std::move(*video), std::move(*device), std::move(*store)};
}

// The frames of the foundation screen, each one complete: its ticks, its snapshot and its drawing without a
// window.
class FrameLoop {
public:
    FrameLoop(FoundationScreen& screen, nexum::render::OffscreenTarget& target,
              const nexum::render::TextureStore& store) noexcept
        : m_screen(&screen), m_target(&target), m_store(&store) {}

    // Runs `count` frames and returns how many failed: a plain number, so that keeping it asks for no memory.
    [[nodiscard]] std::uint32_t Run(int count) {
        std::uint32_t failed = 0;
        for (int frame = 0; frame < count; ++frame) {
            const Seconds frameTime{kFrameTimes[m_next]};
            m_next = (m_next + 1) % kFrameTimes.size();
            m_now += frameTime;
            const bool framed =
                m_screen->Frame(m_now, frameTime, mu::foundation::kProofSceneSize).has_value() &&
                m_target->Draw(m_screen->Drawing(), *m_store).has_value();
            if (!framed) {
                ++failed;
            }
        }
        return failed;
    }

private:
    FoundationScreen* m_screen;
    nexum::render::OffscreenTarget* m_target;
    const nexum::render::TextureStore* m_store;
    Seconds m_now{0.0};
    std::size_t m_next = 0;
};

} // namespace

// The counter sees the requests made through new on the thread of the frame. What SDL and the video drivers
// ask for through malloc is not counted on Windows and Linux (docs/TECH_DEBT.md, TD-0019).
TEST_CASE("[0000-CA-09] after 3 frames of warm-up, 1000 frames of the foundation screen ask for no memory") {
    Gpu gpu = StartGpu();
    CountingLogSink sink;
    nexum::debug::Logger log(sink);
    auto screen = FoundationScreen::Create(gpu.store, log, Seconds{0.0});
    REQUIRE_MESSAGE(screen.has_value(), (screen ? "" : screen.error().message));
    auto target = nexum::render::OffscreenTarget::Create(gpu.device, mu::foundation::kProofSceneSize);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));

    FrameLoop loop(*screen, *target, gpu.store);
    REQUIRE(loop.Run(kWarmUpFrames) == 0);

    const std::uint64_t ticksBefore = screen->Ticks().ElapsedTicks();
    const std::uint64_t messagesBefore = sink.Messages();
    std::uint32_t failed = 0;
    AllocationTally tally;
    {
        const AllocationCounter counter;
        failed = loop.Run(kMeasuredFrames);
        tally = counter.Tally();
    }

    // On Linux and macOS, a failure also says where the first requests came from.
    const std::string origins = frame_allocations::DescribeCountedRequests();
    CHECK(failed == 0);
    CHECK_MESSAGE(tally.requests == 0, tally.requests, " requests for memory (", tally.bytes, " bytes) in ",
                  kMeasuredFrames, " frames\n", origins);
    // The measured frames went through every part of the frame: more than a thousand ticks, and the reports
    // of the tick loop in the log.
    CHECK(screen->Ticks().ElapsedTicks() - ticksBefore > 1000);
    CHECK(sink.Messages() - messagesBefore >= 10);
}

TEST_CASE("[0000-CA-09] the counter sees each request for memory of its thread while it exists") {
    struct alignas(64) WideBlock {
        std::array<std::byte, 64> bytes{};
    };

    AllocationTally tally;
    {
        const AllocationCounter counter;
        const auto number = std::make_unique<std::uint64_t>(7);
        KeepAddress(number.get());
        const auto numbers = std::make_unique<std::uint32_t[]>(100);
        KeepAddress(numbers.get());
        const auto wide = std::make_unique<WideBlock>();
        KeepAddress(wide.get());
        tally = counter.Tally();
    }
    CHECK(tally.requests == 3);
    CHECK(tally.bytes == sizeof(std::uint64_t) + (100 * sizeof(std::uint32_t)) + sizeof(WideBlock));

    // On Linux and macOS, each of the 3 requests has its size and its call stack.
    const std::string origins = frame_allocations::DescribeCountedRequests();
    if constexpr (frame_allocations::kRecordsCallStacks) {
        CHECK(origins.starts_with("request 1 (8 bytes):\n  "));
        CHECK(origins.find("request 2 (400 bytes):\n  ") != std::string::npos);
        CHECK(origins.find("request 3 (64 bytes):\n  ") != std::string::npos);
        CHECK(origins.find("request 4") == std::string::npos);
    } else {
        CHECK(origins.empty());
    }

    // Without a counter nothing is counted, and a new counter starts from zero.
    const auto uncounted = std::make_unique<std::uint64_t>(8);
    KeepAddress(uncounted.get());
    AllocationTally fresh;
    {
        const AllocationCounter counter;
        fresh = counter.Tally();
    }
    CHECK(fresh.requests == 0);
    CHECK(fresh.bytes == 0);
}
