// Fuzz target of the reading of a .mucap recording and of its replay (cmake/NexumFuzzing.cmake, and CA-09 of
// slice 2): any bytes give a recording or the reason of the refusal, never a crash. A valid recording is then
// replayed whole by the online client, with a clock that jumps past the end of the recording at once. The
// replay hands over at most one group with events in each frame, so it ends, with the end or a difference, in
// at most as many frames as the recording has records, plus 2; a replay that reached the end checked every
// message, tick and checkpoint of the recording; and a failed one says why.
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Online/ReplayServer.hpp"
#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Recording/Recording.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace {

namespace recording = mu::recording;
using mu::online::OnlineClient;
using mu::online::OnlineStatus;
using mu::online::ReplayServer;
using mu::online::ServerState;
using nexum::time::Seconds;

// A log output that keeps nothing: the lines are still built, so building them is fuzzed too.
class SilentLogSink final : public nexum::debug::LogSink {
public:
    void Write(const nexum::debug::LogRecord& /*record*/) noexcept override {}
};

// Keeps the values that were read, so the reading cannot be left out of the build.
class Sink {
public:
    void Take(std::uint64_t value) noexcept { m_sum = m_sum + value; }

private:
    volatile std::uint64_t m_sum = 0;
};

[[noreturn]] void Fail(const char* what) {
    std::fputs(what, stderr);
    std::fputs("\n", stderr);
    std::abort();
}

// How many records the recording has, and the time of the last one, the end.
struct Extent {
    std::size_t records = 0;
    recording::Microseconds end = 0;
};

[[nodiscard]] Extent ExtentOf(const recording::Recording& recorded) noexcept {
    Extent extent;
    for (auto cursor = recorded.Records(); !cursor.AtEnd(); ++extent.records) {
        const recording::Record record = cursor.Next();
        extent.end = std::visit([](const auto& any) { return any.time; }, record);
    }
    return extent;
}

void Replay(const recording::Recording& recorded, Sink& sink) {
    SilentLogSink silent;
    nexum::debug::Logger log{silent, nexum::debug::LogLevel::Trace};
    ReplayServer server{recorded, log};
    OnlineClient client{server, log, std::nullopt};

    // The clock jumps to a second after the end, so every group is due in the first frame it can come.
    const Extent extent = ExtentOf(recorded);
    const Seconds now =
        std::chrono::duration<double, std::micro>(static_cast<double>(extent.end)) + Seconds{1.0};
    client.Start(Seconds{0.0});
    for (std::size_t frames = 0; client.Status(now) == OnlineStatus::Running; ++frames) {
        if (frames == extent.records + 2) {
            Fail("the replay of a valid recording did not end in as many frames as it has records, plus 2");
        }
        client.TakeEvents(now);
        client.AdvanceClock(now);
        client.ApplyEvents(now);
    }

    const auto status = server.Status();
    if (status.state == ServerState::Ended &&
        (server.Messages() != recorded.Messages() || server.Ticks() != recorded.Ticks() ||
         server.Checkpoints() != recorded.Checkpoints())) {
        Fail("the replay reached the end without checking every message, tick and checkpoint");
    }
    if (status.state == ServerState::Failed && status.reason.empty()) {
        Fail("the replay failed without saying why");
    }
    sink.Take(client.Summary(now).size());
    sink.Take(client.DroppedEvents());
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    Sink sink;
    const auto recorded = recording::ParseRecording(std::vector<std::byte>(bytes.begin(), bytes.end()));
    if (!recorded) {
        if (recorded.error().message.empty()) {
            Fail("the reading refused a recording without saying why");
        }
        return 0;
    }
    sink.Take(recorded->Header().nexumVersion.size());
    Replay(*recorded, sink);
    return 0;
}
