// The replay of slice 2 (CA-05), with the online client and a clock of test: each group of messages handed
// over when the time of its tick came, and the replay waiting for that tick; a tick of the client that takes
// nothing, a frame without a tick and a frame with several; a group whose messages give no events; a change
// of epoch between the translation and the tick; each difference, with the tick or the checkpoint and both
// values; the end; and a session recorded live, replayed to the same view at three paces, with the same
// result each time. The recordings made by hand get their numbers from a translation and a view of their
// own, which play the recorded session.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Online/LiveServer.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Online/ReplayServer.hpp"
#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Recording/Recorder.hpp"
#include "Mu/Recording/Recording.hpp"
#include "Mu/Recording/StateDigest.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "ScriptedTransport.hpp"
#include "SessionMessages.hpp"

namespace {

namespace recording = mu::recording;
namespace world = mu::world;
using mu::online::OnlineClient;
using mu::online::OnlineStatus;
using mu::online::ReplayServer;
using mu::online::ServerState;
using mu::session::SessionEpoch;
using nexum::debug::LogLevel;
using nexum::time::Seconds;
using recording::Checkpoint;
using recording::EndReason;
using recording::EnteredWorld;
using recording::EpochChanged;
using recording::Record;
using recording::RecordingEnd;
using recording::TickMark;
using recording::WorldMessage;
using namespace session_messages;

constexpr world::ObjectId kHero{0x0200};
// The epoch of the Game Server, the second connection.
constexpr SessionEpoch kGameServer{2};
constexpr std::size_t kRoom = std::size_t{64} * 1024;
// More frames than any replay of these tests needs.
constexpr std::size_t kMostFrames = 2000;

[[nodiscard]] recording::RecordingHeader TestHeader() {
    return {.nexumVersion = "0.3.0", .dialectVersion = "20404", .server = "OpenMU 0.10.1"};
}

[[nodiscard]] recording::Recording ReadBack(std::span<const std::byte> bytes) {
    auto read = recording::ParseRecording(Bytes(bytes.begin(), bytes.end()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    return std::move(*read);
}

// A recording made by hand, record by record, then read with every check of the format.
class HandMade {
public:
    HandMade() : m_writer(MakeWriter()) {}

    HandMade& Add(const Record& record) {
        REQUIRE(m_writer.Write(record));
        return *this;
    }

    [[nodiscard]] recording::Recording Read() const { return ReadBack(m_writer.Bytes()); }

private:
    [[nodiscard]] static recording::RecordingWriter MakeWriter() {
        auto writer = recording::RecordingWriter::Create(TestHeader(), kRoom);
        REQUIRE_MESSAGE(writer.has_value(), (writer ? "" : writer.error().message));
        return std::move(*writer);
    }

    recording::RecordingWriter m_writer;
};

// The recorded session, played by a translation and a view of its own: how many events each tick took, and
// the checkpoint after it, for the recordings made by hand.
struct RecordedSide {
    // Translates the message; the next tick takes its events.
    void Receive(SessionEpoch epoch, const Bytes& message) { translator.Translate(epoch, message, queue); }

    // Takes the events out of the queue, applying those of the current epoch; gives how many it took.
    [[nodiscard]] std::uint32_t Tick(SessionEpoch current) {
        const std::size_t taken =
            queue.Drain([this, current](SessionEpoch epoch, const world::WorldEvent& event) {
                if (epoch >= current) {
                    view.Apply(event);
                }
            });
        return static_cast<std::uint32_t>(taken);
    }

    [[nodiscard]] Checkpoint CheckpointAt(std::uint64_t time, std::uint32_t messages) const {
        return {.time = time,
                .messages = messages,
                .digest = recording::StateDigestOf(view),
                .objects = static_cast<std::uint32_t>(view.Objects().size()),
                .items = static_cast<std::uint32_t>(view.Inventory().size())};
    }

    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    mu::translate::Translator translator{kHero, log};
    world::EventQueue queue{64};
    world::WorldView view{log};
};

// The replay server, the online client and the log.
struct Replay {
    explicit Replay(const recording::Recording& recorded) : server(recorded, log) {}

    // Steps 2 and 3 of a frame.
    void Frame(double now) {
        client.TakeEvents(Seconds{now});
        client.AdvanceClock(Seconds{now});
    }

    // The start of a tick.
    void Tick(double now) { client.ApplyEvents(Seconds{now}); }

    // The lines of the log of the replay, or of another category, at one level.
    [[nodiscard]] std::vector<std::string> Lines(LogLevel level, std::string_view category = "replay") const {
        std::vector<std::string> lines;
        for (const auto& entry : sink.Entries()) {
            if (entry.category == category && entry.level == level) {
                lines.push_back(entry.message);
            }
        }
        return lines;
    }

    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ReplayServer server;
    OnlineClient client{server, log, std::nullopt};
};

[[nodiscard]] std::string Hex(std::uint64_t value) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::string text(16, '0');
    for (std::size_t digit = 0; digit < text.size(); ++digit) {
        text[text.size() - 1 - digit] = kDigits[(value >> (4U * digit)) & 0x0FU];
    }
    return text;
}

// "1 object", "2 objects".
[[nodiscard]] std::string Count(std::uint64_t count, std::string_view thing) {
    return std::to_string(count) + " " + std::string(thing) + (count == 1 ? "" : "s");
}

[[nodiscard]] std::string StateText(std::uint64_t digest, std::uint64_t objects, std::uint64_t items) {
    return "the digest " + Hex(digest) + " with " + Count(objects, "object") + " and " + Count(items, "item");
}

// The recording of most tests: the entry in the world with the data of the character, at 0 s; an NPC and the
// player test0Dk coming into view, at 0.3 and 0.32 s, for the tick at 0.34 s, which has a checkpoint; and the
// end at 0.4 s. The numbers of the second tick and its checkpoint can be changed, to make a difference.
struct TwoTicks {
    TwoTicks() {
        side.Receive(kGameServer, data);
        first = side.Tick(kGameServer);
        side.Receive(kGameServer, npc);
        side.Receive(kGameServer, hero);
        second = side.Tick(kGameServer);
        checkpoint = side.CheckpointAt(340'000, 3);
    }

    [[nodiscard]] recording::Recording Read() const {
        return HandMade()
            .Add(EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero})
            .Add(WorldMessage{.time = 0, .epoch = kGameServer, .bytes = data})
            .Add(TickMark{.time = 0, .events = first})
            .Add(WorldMessage{.time = 300'000, .epoch = kGameServer, .bytes = npc})
            .Add(WorldMessage{.time = 320'000, .epoch = kGameServer, .bytes = hero})
            .Add(TickMark{.time = 340'000, .events = second})
            .Add(checkpoint)
            .Add(RecordingEnd{.time = 400'000, .reason = EndReason::Closed})
            .Read();
    }

    RecordedSide side;
    Bytes data = CharacterInformation();
    Bytes npc = NpcCameIntoView();
    Bytes hero = HeroEntered();
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    Checkpoint checkpoint;
};

// A session recorded live: the live server with a recorder, the transport in memory and the online client.
// The recorder tests (RecorderTest.cpp) have the first copy of this way to the world; this is the second.
struct LiveSession {
    LiveSession() : recorder(MakeRecorder(log)), server(Options(), transport, log, &recorder) {}

    [[nodiscard]] static recording::Recorder MakeRecorder(nexum::debug::Logger& log) {
        auto made = recording::Recorder::Create(TestHeader(), kRoom, log);
        REQUIRE_MESSAGE(made.has_value(), (made ? "" : made.error().message));
        return std::move(*made);
    }

    [[nodiscard]] static mu::session::SessionOptions Options() {
        mu::session::SessionOptions options;
        options.account = "test0";
        options.password = "test0";
        return options;
    }

    void Frame(double now) {
        client.TakeEvents(Seconds{now});
        client.AdvanceClock(Seconds{now});
    }

    void Tick(double now) { client.ApplyEvents(Seconds{now}); }

    // One answer of the server for each frame, 0.1 s apart: the entry in the world is in the frame at 0.7 s.
    void WalkToTheWorld() {
        client.Start(Seconds{0.0});
        const std::vector<Bytes> answers{Hello(),
                                         ServerList(),
                                         ConnectionInfo(),
                                         GameServerEntered(),
                                         LoginResponse(1),
                                         CharacterList({"test0Dk"}),
                                         CharacterInformation()};
        for (std::size_t step = 0; step < answers.size(); ++step) {
            if (step == 0 || step == 3) {
                transport.Connected();
            }
            transport.Message(answers[step]);
            Frame(0.1 * static_cast<double>(step + 1));
        }
        REQUIRE(server.State() == mu::session::SessionState::InWorld);
    }

    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    recording::Recorder recorder;
    test_transport::ScriptedTransport transport;
    mu::online::LiveServer server;
    OnlineClient client{server, log, std::nullopt};
};

// The recording of a live session and the view it ended with.
struct LiveRecording {
    recording::Recording recorded;
    std::uint64_t digest = 0;
    std::size_t objects = 0;
};

// The session of the recorder tests: 6 messages in 5 ticks, a frame without a tick, the checkpoint at 5 s in
// the world and the one of the end. As the MuClient does, the last tick comes before the end.
[[nodiscard]] LiveRecording RecordLive() {
    LiveSession live;
    live.WalkToTheWorld();
    live.Tick(0.7);
    live.transport.Message(HeroEntered());
    live.transport.Message(NpcCameIntoView());
    live.Frame(1.0);
    live.Frame(1.02);
    live.Tick(1.04);
    live.transport.Message(CurrentStats());
    live.Frame(5.6);
    live.Tick(5.6);
    live.transport.Message(HeroWalked());
    live.Frame(5.8);
    live.Tick(5.8);
    live.transport.Message(NpcLeftTheView());
    live.Frame(7.0);
    live.Tick(7.0);
    live.Tick(7.5);
    live.recorder.Finish(live.client.View(), Seconds{7.5}, EndReason::Closed);
    REQUIRE(live.recorder.Ticks() == 5);
    REQUIRE(live.recorder.Checkpoints() == 2);
    return {.recorded = ReadBack(live.recorder.Bytes()),
            .digest = recording::StateDigestOf(live.client.View()),
            .objects = live.client.View().Objects().size()};
}

// What a whole replay gave.
struct Outcome {
    OnlineStatus status = OnlineStatus::Running;
    std::string summary;
    std::uint64_t digest = 0;
    std::size_t objects = 0;
    std::uint64_t dropped = 0;
    std::vector<std::string> lines;
    // What the view of the world said in the log.
    std::vector<std::string> world;

    friend bool operator==(const Outcome&, const Outcome&) = default;
};

// The whole replay: a frame every `step` seconds of the clock of test, with the ticks of `ticksPerFrame` in
// turn, until the client is no longer running.
[[nodiscard]] Outcome ReplayAtPace(const recording::Recording& recorded, double step,
                                   std::span<const int> ticksPerFrame) {
    Replay replay(recorded);
    double now = 0.0;
    replay.client.Start(Seconds{now});
    for (std::size_t frame = 0;
         frame < kMostFrames && replay.client.Status(Seconds{now}) == OnlineStatus::Running; ++frame) {
        now += step;
        replay.Frame(now);
        for (int tick = 0; tick < ticksPerFrame[frame % ticksPerFrame.size()]; ++tick) {
            replay.Tick(now);
        }
    }
    return {.status = replay.client.Status(Seconds{now}),
            .summary = replay.server.Summary(Seconds{now}),
            .digest = recording::StateDigestOf(replay.client.View()),
            .objects = replay.client.View().Objects().size(),
            .dropped = replay.client.DroppedEvents(),
            .lines = replay.Lines(LogLevel::Info),
            .world = replay.Lines(LogLevel::Info, "world")};
}

// The bytes of a file, as they are on the disk.
[[nodiscard]] Bytes FileBytes(const char* path) {
    std::ifstream input(path, std::ios::binary);
    REQUIRE_MESSAGE(input.good(), "could not open ", path);
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    Bytes bytes;
    bytes.reserve(text.size());
    for (const char letter : text) {
        bytes.push_back(static_cast<std::byte>(letter));
    }
    return bytes;
}

// Each place where `text` is in `bytes`.
[[nodiscard]] std::vector<std::size_t> PlacesOf(const Bytes& bytes, std::string_view text) {
    std::vector<std::size_t> places;
    const auto matches = [](std::byte byte, char letter) { return byte == static_cast<std::byte>(letter); };
    for (auto at = bytes.begin();
         (at = std::search(at, bytes.end(), text.begin(), text.end(), matches)) != bytes.end(); ++at) {
        places.push_back(static_cast<std::size_t>(at - bytes.begin()));
    }
    return places;
}

} // namespace

TEST_CASE("[0002-CA-05] the replay hands over each group when the time of its tick came, then waits for it") {
    const TwoTicks recorded;
    REQUIRE(recorded.first > 0);
    REQUIRE(recorded.second > 0);
    const recording::Recording recording = recorded.Read();
    Replay replay(recording);

    // Nothing before the start.
    replay.Frame(1.0);
    CHECK(replay.client.PendingEvents() == 0);
    CHECK(replay.server.Status().state == ServerState::Connecting);

    // The times of the recording count from the start, here at 10 s.
    replay.client.Start(Seconds{10.0});
    CHECK(replay.Lines(LogLevel::Info) ==
          std::vector<std::string>{
              "replaying 3 messages, 2 ticks and 1 checkpoint, recorded by the Nexum 0.3.0 "
              "with the server OpenMU 0.10.1"});
    replay.Frame(10.0);
    CHECK(replay.server.Status().state == ServerState::InWorld);
    CHECK(replay.server.CurrentEpoch() == kGameServer);
    CHECK(replay.client.PendingEvents() == recorded.first);
    replay.Tick(10.0);
    CHECK(replay.server.Ticks() == 1);

    // The messages of the next group came at 0.3 and 0.32 s, but its tick only at 0.34 s.
    replay.Frame(10.339);
    CHECK(replay.client.PendingEvents() == 0);
    CHECK(replay.server.Messages() == 1);
    // A tick of the client that takes nothing is no tick of the recording.
    replay.Tick(10.339);
    CHECK(replay.server.Ticks() == 1);

    replay.Frame(10.34);
    CHECK(replay.client.PendingEvents() == recorded.second);
    CHECK(replay.server.Messages() == 3);
    replay.Tick(10.34);
    CHECK(replay.server.Ticks() == 2);
    CHECK(replay.server.Checkpoints() == 1);
    CHECK(recording::StateDigestOf(replay.client.View()) == recorded.checkpoint.digest);

    replay.Frame(10.399);
    CHECK(replay.server.Status().state == ServerState::InWorld);
    replay.Frame(10.4);
    CHECK(replay.server.Status().state == ServerState::Ended);
    CHECK(replay.client.Status(Seconds{10.4}) == OnlineStatus::ServerEnded);
    CHECK(replay.client.Summary(Seconds{10.4}) ==
          "the session ends: 0 s in the world, the replay checked 3 messages, 2 ticks and 1 checkpoint, "
          "objects in view: " +
              std::to_string(recorded.side.view.Objects().size()));
    CHECK(replay.Lines(LogLevel::Info) ==
          std::vector<std::string>{
              "replaying 3 messages, 2 ticks and 1 checkpoint, recorded by the Nexum 0.3.0 "
              "with the server OpenMU 0.10.1",
              "the replay is in the world, with the hero 0200",
              "the replay reached the end of the recording, where the client ended it: every "
              "tick and checkpoint matched"});
    CHECK(replay.Lines(LogLevel::Error).empty());
}

TEST_CASE(
    "[0002-CA-05] a frame without a tick keeps the group waiting, and the next group waits for that tick") {
    const TwoTicks recorded;
    const recording::Recording recording = recorded.Read();
    Replay replay(recording);
    replay.client.Start(Seconds{0.0});

    // The time of both groups passed, but the second waits for the tick of the first.
    replay.Frame(0.0);
    replay.Frame(0.35);
    CHECK(replay.client.PendingEvents() == recorded.first);
    CHECK(replay.server.Messages() == 1);
    replay.Tick(0.35);
    CHECK(replay.server.Ticks() == 1);

    replay.Frame(0.36);
    CHECK(replay.client.PendingEvents() == recorded.second);
    replay.Tick(0.36);
    CHECK(replay.server.Ticks() == 2);
    CHECK(replay.server.Checkpoints() == 1);
    // The end waited for its own frame.
    CHECK(replay.server.Status().state == ServerState::InWorld);
    replay.Frame(0.4);
    CHECK(replay.server.Status().state == ServerState::Ended);
}

TEST_CASE(
    "[0002-CA-05] in a frame with several ticks, the first takes the group and the others take nothing") {
    const TwoTicks recorded;
    const recording::Recording recording = recorded.Read();
    Replay replay(recording);
    replay.client.Start(Seconds{0.0});

    replay.Frame(0.5);
    replay.Tick(0.5);
    replay.Tick(0.5);
    replay.Tick(0.5);
    CHECK(replay.server.Ticks() == 1);
    CHECK(replay.server.Status().state == ServerState::InWorld);

    replay.Frame(0.6);
    replay.Tick(0.6);
    replay.Tick(0.6);
    CHECK(replay.server.Ticks() == 2);
    CHECK(replay.server.Checkpoints() == 1);
    replay.Frame(0.7);
    CHECK(replay.server.Status().state == ServerState::Ended);
    CHECK(replay.Lines(LogLevel::Error).empty());
}

TEST_CASE("[0002-CA-05] a tick that took another number of events ends the replay, with the tick and both "
          "numbers") {
    TwoTicks recorded;
    const std::uint32_t replayed = recorded.second;
    recorded.second += 1;
    const recording::Recording recording = recorded.Read();
    Replay replay(recording);
    replay.client.Start(Seconds{0.0});
    replay.Frame(0.0);
    replay.Tick(0.0);
    replay.Frame(0.34);
    replay.Tick(0.34);

    const std::string expected = "the tick 2 at 340000 us took " + Count(recorded.second, "event") +
                                 " in the recording, but " + std::to_string(replayed) + " in the replay";
    CHECK(replay.server.Status().state == ServerState::Failed);
    CHECK(replay.server.Status().reason == expected);
    CHECK(replay.client.Status(Seconds{0.34}) == OnlineStatus::Failed);
    CHECK(replay.Lines(LogLevel::Error) == std::vector<std::string>{expected});
    // The checkpoint after the tick is not checked, and the replay stops there.
    CHECK(replay.server.Checkpoints() == 0);
    replay.Frame(0.4);
    CHECK(replay.server.Status().state == ServerState::Failed);
    CHECK(replay.server.Summary(Seconds{0.4}) == "the replay checked 3 messages, 1 tick and 0 checkpoints");
}

TEST_CASE(
    "[0002-CA-05] a checkpoint with another view ends the replay, with the checkpoint and both states") {
    TwoTicks recorded;
    const Checkpoint replayed = recorded.checkpoint;
    SUBCASE("the digest") {
        recorded.checkpoint.digest += 1;
    }
    SUBCASE("the objects") {
        recorded.checkpoint.objects += 1;
    }
    SUBCASE("the items") {
        recorded.checkpoint.items += 1;
    }
    const recording::Recording recording = recorded.Read();
    Replay replay(recording);
    replay.client.Start(Seconds{0.0});
    replay.Frame(0.0);
    replay.Tick(0.0);
    replay.Frame(0.34);
    replay.Tick(0.34);

    const Checkpoint& wrong = recorded.checkpoint;
    const std::string expected =
        "the checkpoint 1 at 340000 us has " + StateText(wrong.digest, wrong.objects, wrong.items) +
        " in the recording, but " + StateText(replayed.digest, replayed.objects, replayed.items) +
        " in the replay";
    CHECK(replay.server.Status().state == ServerState::Failed);
    CHECK(replay.server.Status().reason == expected);
    CHECK(replay.Lines(LogLevel::Error) == std::vector<std::string>{expected});
    CHECK(replay.server.Ticks() == 2);
    CHECK(replay.server.Checkpoints() == 0);
}

TEST_CASE("[0002-CA-05] a change of epoch between the translation and the tick drops the same events") {
    RecordedSide side;
    const Bytes data = CharacterInformation();
    const Bytes npc = NpcCameIntoView();
    const Bytes hero = HeroEntered();
    side.Receive(kGameServer, data);
    const std::uint32_t first = side.Tick(kGameServer);
    // The NPC comes in the epoch 2, and the epoch is 3 when the tick takes it: it never reaches the view.
    side.Receive(kGameServer, npc);
    const std::uint32_t dropped = side.Tick(SessionEpoch{3});
    const Checkpoint afterTheDrop = side.CheckpointAt(120'000, 2);
    side.Receive(SessionEpoch{3}, hero);
    const std::uint32_t third = side.Tick(SessionEpoch{3});
    REQUIRE(dropped > 0);
    REQUIRE(side.view.Find(world::ObjectId{0x0021}) == nullptr);

    const recording::Recording recording =
        HandMade()
            .Add(EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero})
            .Add(WorldMessage{.time = 0, .epoch = kGameServer, .bytes = data})
            .Add(TickMark{.time = 0, .events = first})
            .Add(WorldMessage{.time = 100'000, .epoch = kGameServer, .bytes = npc})
            .Add(EpochChanged{.time = 120'000, .epoch = SessionEpoch{3}})
            .Add(TickMark{.time = 120'000, .events = dropped})
            .Add(afterTheDrop)
            .Add(WorldMessage{.time = 200'000, .epoch = SessionEpoch{3}, .bytes = hero})
            .Add(TickMark{.time = 200'000, .events = third})
            .Add(RecordingEnd{.time = 300'000, .reason = EndReason::Closed})
            .Read();
    constexpr std::array kOneTick{1};
    const Outcome outcome = ReplayAtPace(recording, 0.04, kOneTick);
    CHECK(outcome.status == OnlineStatus::ServerEnded);
    CHECK(outcome.dropped == dropped);
    CHECK(outcome.digest == recording::StateDigestOf(side.view));
    CHECK(outcome.summary == "the replay checked 3 messages, 3 ticks and 1 checkpoint");
}

TEST_CASE("[0002-CA-05] a tick of the recording whose messages give no events is checked at once") {
    RecordedSide side;
    const Bytes data = CharacterInformation();
    const EnteredWorld entry{.time = 0, .epoch = kGameServer, .hero = kHero};
    const auto replayOf = [](const recording::Recording& recording) {
        auto replay = std::make_unique<Replay>(recording);
        replay->client.Start(Seconds{0.0});
        replay->Frame(0.1);
        return replay;
    };

    SUBCASE("a mark of 0 events passes, and the next group comes in the same frame") {
        side.Receive(kGameServer, data);
        const std::uint32_t events = side.Tick(kGameServer);
        const recording::Recording recording =
            HandMade()
                .Add(entry)
                .Add(TickMark{.time = 50'000, .events = 0})
                .Add(WorldMessage{.time = 60'000, .epoch = kGameServer, .bytes = data})
                .Add(TickMark{.time = 60'000, .events = events})
                .Add(side.CheckpointAt(60'000, 1))
                .Add(RecordingEnd{.time = 60'000, .reason = EndReason::Closed})
                .Read();
        const auto replay = replayOf(recording);
        CHECK(replay->server.Ticks() == 1);
        CHECK(replay->client.PendingEvents() == events);
        replay->Tick(0.1);
        replay->Frame(0.2);
        CHECK(replay->server.Status().state == ServerState::Ended);
        CHECK(replay->server.Summary(Seconds{0.2}) ==
              "the replay checked 1 message, 2 ticks and 1 checkpoint");
    }
    SUBCASE("a mark of more events is a difference") {
        const recording::Recording recording =
            HandMade()
                .Add(entry)
                .Add(TickMark{.time = 50'000, .events = 3})
                .Add(RecordingEnd{.time = 60'000, .reason = EndReason::Closed})
                .Read();
        const auto replay = replayOf(recording);
        CHECK(replay->server.Status().reason ==
              "the tick 1 at 50000 us took 3 events in the recording, but 0 in "
              "the replay");
    }
    SUBCASE("a checkpoint after a mark of 0 events is a difference") {
        const recording::Recording recording =
            HandMade()
                .Add(entry)
                .Add(TickMark{.time = 50'000, .events = 0})
                .Add(side.CheckpointAt(50'000, 0))
                .Add(RecordingEnd{.time = 60'000, .reason = EndReason::Closed})
                .Read();
        const auto replay = replayOf(recording);
        CHECK(replay->server.Status().state == ServerState::Failed);
        CHECK(replay->server.Status().reason ==
              "the checkpoint 1 at 50000 us follows a tick without events, "
              "which never reaches the replay, so the view cannot be compared");
        CHECK(replay->server.Ticks() == 1);
    }
}

TEST_CASE("[0002-CA-05] the end: at its time, after the last tick, and nothing is checked after it") {
    SUBCASE("a session that never reached the world") {
        const recording::Recording recording =
            HandMade().Add(RecordingEnd{.time = 500'000, .reason = EndReason::SessionFailed}).Read();
        Replay replay(recording);
        replay.client.Start(Seconds{0.0});
        replay.Frame(0.499);
        CHECK(replay.server.Status().state == ServerState::Connecting);
        replay.Frame(0.5);
        CHECK(replay.client.Status(Seconds{0.5}) == OnlineStatus::ServerEnded);
        CHECK(
            replay.client.Summary(Seconds{0.5}) ==
            "the session ends: never in the world, the replay checked 0 messages, 0 ticks and 0 checkpoints");
        CHECK(replay.Lines(LogLevel::Info).back() == "the replay reached the end of the recording, where the "
                                                     "session failed: every tick and checkpoint "
                                                     "matched");
    }
    SUBCASE("the messages after the last tick come with the end, and their tick is not checked") {
        RecordedSide side;
        const Bytes data = CharacterInformation();
        const Bytes npc = NpcCameIntoView();
        side.Receive(kGameServer, data);
        const std::uint32_t events = side.Tick(kGameServer);
        const recording::Recording recording =
            HandMade()
                .Add(EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero})
                .Add(WorldMessage{.time = 0, .epoch = kGameServer, .bytes = data})
                .Add(TickMark{.time = 0, .events = events})
                .Add(WorldMessage{.time = 200'000, .epoch = kGameServer, .bytes = npc})
                .Add(RecordingEnd{.time = 300'000, .reason = EndReason::Full})
                .Read();
        Replay replay(recording);
        replay.client.Start(Seconds{0.0});
        replay.Frame(0.0);
        replay.Tick(0.0);
        replay.Frame(0.299);
        CHECK(replay.client.PendingEvents() == 0);
        replay.Frame(0.3);
        CHECK(replay.client.PendingEvents() > 0);
        CHECK(replay.server.Status().state == ServerState::Ended);
        replay.Tick(0.3);
        replay.Frame(0.4);
        replay.Tick(0.4);
        CHECK(replay.client.Status(Seconds{0.4}) == OnlineStatus::ServerEnded);
        CHECK(replay.server.Summary(Seconds{0.4}) ==
              "the replay checked 2 messages, 1 tick and 0 checkpoints");
        CHECK(replay.Lines(LogLevel::Info).back() == "the replay reached the end of the recording, where its "
                                                     "room filled up: every tick and checkpoint "
                                                     "matched");
    }
}

TEST_CASE("[0002-CA-05] a session recorded live is replayed without a server to the same view, at any pace") {
    const LiveRecording live = RecordLive();

    // A frame every 20 ms with a tick in every other one, as at 50 frames per second; frames of 100 ms with 2
    // or 3 ticks; and a clock that jumps 1000 s at each frame. The 7 s of the recording take milliseconds.
    constexpr std::array kEveryOtherFrame{1, 0};
    constexpr std::array kSeveralTicks{2, 3};
    constexpr std::array kOneTick{1};
    const Outcome steady = ReplayAtPace(live.recorded, 0.02, kEveryOtherFrame);
    CHECK(steady.status == OnlineStatus::ServerEnded);
    CHECK(steady.summary == "the replay checked 6 messages, 5 ticks and 2 checkpoints");
    CHECK(steady.digest == live.digest);
    CHECK(steady.objects == live.objects);
    CHECK(steady.lines.size() == 3);

    CHECK(ReplayAtPace(live.recorded, 0.1, kSeveralTicks) == steady);
    CHECK(ReplayAtPace(live.recorded, 1000.0, kOneTick) == steady);
    // Two replays of the same recording give the same result.
    CHECK(ReplayAtPace(live.recorded, 0.02, kEveryOtherFrame) == steady);
}

// The reference recording (Recordings/openmu-two-players.mucap): about 20 s in the world of the local OpenMU
// 0.10.1 with test0Dk, the character of the account test0, while test1Dk, of the account test1, appears in
// view and leaves it. To make it again, see docs/setup/DEV.md.
TEST_CASE("[0002-CA-07] the reference recording of the local OpenMU replays to its end at any pace, and has "
          "neither an account nor a password") {
    auto read = recording::ReadRecording(NEXUM_REFERENCE_RECORDING);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    const recording::Recording& recorded = *read;
    CHECK(recorded.Header().dialectVersion == "20404");
    CHECK(recorded.Header().server == "OpenMU 0.10.1");
    CHECK(recorded.Messages() == 23);
    CHECK(recorded.Ticks() == 6);
    CHECK(recorded.Checkpoints() == 2);
    CHECK(recorded.End() == EndReason::Closed);

    // At 50 frames per second with a tick in every other frame, at 10 frames per second with 2 or 3 ticks,
    // and with a clock that jumps 1000 s at each frame: every tick and checkpoint matches, with the same
    // result.
    constexpr std::array kEveryOtherFrame{1, 0};
    constexpr std::array kSeveralTicks{2, 3};
    constexpr std::array kOneTick{1};
    const Outcome steady = ReplayAtPace(recorded, 0.02, kEveryOtherFrame);
    CHECK(steady.status == OnlineStatus::ServerEnded);
    CHECK(steady.summary == "the replay checked 23 messages, 6 ticks and 2 checkpoints");
    CHECK(steady.objects == 6);
    CHECK(steady.dropped == 0);
    CHECK(steady.lines.back() == "the replay reached the end of the recording, where the client ended it: "
                                 "every tick and checkpoint matched");
    const auto said = [&steady](std::string_view line) {
        return std::ranges::find(steady.world, line) != steady.world.end();
    };
    CHECK(said("the player test1Dk (0x033E) appeared at (135, 125)"));
    CHECK(said("the player test1Dk (0x033E) left the view"));
    CHECK(ReplayAtPace(recorded, 0.1, kSeveralTicks) == steady);
    CHECK(ReplayAtPace(recorded, 1000.0, kOneTick) == steady);

    // The account and the password of a test account are both its name. In the file it is only the start of
    // the name of its character, in the notices of the server and in the objects in view.
    const Bytes file = FileBytes(NEXUM_REFERENCE_RECORDING);
    for (const std::string_view account : {"test0", "test1"}) {
        const std::vector<std::size_t> places = PlacesOf(file, account);
        CHECK_FALSE(places.empty());
        CHECK(places == PlacesOf(file, std::string(account) + "Dk"));
    }
}

TEST_CASE("[0002-CA-09] a clock past the largest time a recording holds replays it to the end") {
    // The fuzz target jumps the clock past the end of the recording, and the end can be at any time the
    // format holds, up to 2^64 - 1 us: far more than the 2^63 - 1 us of the microseconds of std::chrono.
    const recording::Recording recording =
        HandMade()
            .Add(RecordingEnd{.time = std::numeric_limits<recording::Microseconds>::max(),
                              .reason = EndReason::Closed})
            .Read();
    Replay replay(recording);
    replay.client.Start(Seconds{0.0});
    replay.Frame(1.0e6);
    CHECK(replay.server.Status().state == ServerState::Connecting);

    double past = 0.0;
    SUBCASE("2 * 10^13 s, past 2^64 us") {
        past = 2.0e13;
    }
    SUBCASE("10^13 s, past 2^63 us") {
        past = 1.0e13;
    }
    SUBCASE("a clock at infinity") {
        past = std::numeric_limits<double>::infinity();
    }
    replay.Frame(past);
    CHECK(replay.server.Status().state == ServerState::Ended);
    CHECK(replay.client.Status(Seconds{past}) == OnlineStatus::ServerEnded);
}
