// The recorder of slice 2 (CA-04). Through the live server, with the transport in memory: from the entry in
// the world, each message of the world with the time of its frame and its epoch, the mark of each tick that
// took events, a checkpoint at the first tick from each 5 s in the world on and another at the end; and
// nothing from before the world, nor anything the client sent, so a known account and password are not in the
// file. Alone: the change of epoch, the end, and the room that fills up, after which the recording stays
// valid and ends as full. Each recording is read back by the reading of the format, with all its checks.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Online/LiveServer.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Recording/Recorder.hpp"
#include "Mu/Recording/Recording.hpp"
#include "Mu/Recording/StateDigest.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "ScriptedTransport.hpp"
#include "SessionMessages.hpp"

namespace {

namespace recording = mu::recording;
namespace world = mu::world;
using mu::session::SessionEpoch;
using nexum::debug::LogLevel;
using nexum::time::Seconds;
using recording::Checkpoint;
using recording::EndReason;
using recording::EnteredWorld;
using recording::EpochChanged;
using recording::Record;
using recording::Recorder;
using recording::RecordingEnd;
using recording::TickMark;
using recording::WorldMessage;
using namespace session_messages;

// An account of 10 bytes, the longest, and a password, which the login carries through Xor3 (section 4.4 of
// the protocol spec), so their bytes in the login are not these.
constexpr std::string_view kAccount = "acct7Nexum";
constexpr std::string_view kPassword = "pw9Secret20";

constexpr world::ObjectId kHero{0x0200};
// The epoch of the Game Server, the second connection.
constexpr SessionEpoch kGameServer{2};
constexpr std::size_t kRoom = std::size_t{64} * 1024;
// The header of TestHeader: the mark, the two versions and the three texts with their sizes.
constexpr std::size_t kHeaderSize = 8 + 2 + 2 + (1 + 5) + (1 + 5) + (1 + 13);

[[nodiscard]] recording::RecordingHeader TestHeader() {
    return {.nexumVersion = "0.3.0", .dialectVersion = "20404", .server = "OpenMU 0.10.1"};
}

[[nodiscard]] Recorder MakeRecorder(nexum::debug::Logger& log, std::size_t room = kRoom) {
    auto recorder = Recorder::Create(TestHeader(), room, log);
    REQUIRE_MESSAGE(recorder.has_value(), (recorder ? "" : recorder.error().message));
    return std::move(*recorder);
}

[[nodiscard]] bool Contains(std::span<const std::byte> bytes, std::span<const std::byte> part) {
    return !std::ranges::search(bytes, part).empty();
}

[[nodiscard]] bool Contains(std::span<const std::byte> bytes, std::string_view text) {
    return Contains(bytes, std::as_bytes(std::span(text.data(), text.size())));
}

[[nodiscard]] std::string Hex(std::span<const std::byte> bytes) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::string text;
    for (const std::byte value : bytes) {
        const auto bits = std::to_integer<unsigned>(value);
        text += kDigits[bits >> 4U];
        text += kDigits[bits & 0x0FU];
        text += ' ';
    }
    return text;
}

// One line for each record, so a test compares whole recordings and a difference names its field.
struct Describe {
    [[nodiscard]] std::string operator()(const EnteredWorld& record) const {
        return "entry at " + std::to_string(record.time) + " us, epoch " +
               std::to_string(record.epoch.value) + ", hero " + std::to_string(record.hero.value);
    }
    [[nodiscard]] std::string operator()(const WorldMessage& record) const {
        return "message at " + std::to_string(record.time) + " us, epoch " +
               std::to_string(record.epoch.value) + ": " + Hex(record.bytes);
    }
    [[nodiscard]] std::string operator()(const EpochChanged& record) const {
        return "epoch " + std::to_string(record.epoch.value) + " at " + std::to_string(record.time) + " us";
    }
    [[nodiscard]] std::string operator()(const TickMark& record) const {
        return "tick at " + std::to_string(record.time) + " us, " + std::to_string(record.events) + " events";
    }
    [[nodiscard]] std::string operator()(const Checkpoint& record) const {
        return "checkpoint at " + std::to_string(record.time) + " us, " + std::to_string(record.messages) +
               " messages, digest " + std::to_string(record.digest) + ", " + std::to_string(record.objects) +
               " objects, " + std::to_string(record.items) + " items";
    }
    [[nodiscard]] std::string operator()(const RecordingEnd& record) const {
        return "end at " + std::to_string(record.time) + " us, reason " +
               std::to_string(static_cast<int>(record.reason));
    }
};

// Reads the recording back with every check of the format, and describes its records.
[[nodiscard]] std::vector<std::string> RecordsOf(std::span<const std::byte> bytes) {
    auto read = recording::ParseRecording(Bytes(bytes.begin(), bytes.end()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    std::vector<std::string> lines;
    for (auto cursor = read->Records(); !cursor.AtEnd();) {
        lines.push_back(std::visit(Describe{}, cursor.Next()));
    }
    return lines;
}

[[nodiscard]] std::vector<std::string> Lines(std::initializer_list<Record> records) {
    std::vector<std::string> lines;
    for (const Record& record : records) {
        lines.push_back(std::visit(Describe{}, record));
    }
    return lines;
}

void CheckSame(const std::vector<std::string>& actual, const std::vector<std::string>& expected) {
    CHECK(actual.size() == expected.size());
    for (std::size_t index = 0; index < std::min(actual.size(), expected.size()); ++index) {
        CHECK(actual[index] == expected[index]);
    }
}

[[nodiscard]] Checkpoint CheckpointOf(const world::WorldView& view, std::uint64_t time,
                                      std::uint32_t messages) {
    return {.time = time,
            .messages = messages,
            .digest = recording::StateDigestOf(view),
            .objects = static_cast<std::uint32_t>(view.Objects().size()),
            .items = static_cast<std::uint32_t>(view.Inventory().size())};
}

[[nodiscard]] std::size_t WarningsOf(const nexum::test_support::MemoryLogSink& sink) {
    return static_cast<std::size_t>(std::ranges::count_if(sink.Entries(), [](const auto& entry) {
        return entry.level == LogLevel::Warning && entry.category == "recording";
    }));
}

// The live server with a recorder, the online client, the transport and the log.
struct LiveRecording {
    LiveRecording() : recorder(MakeRecorder(log)), server(Options(), transport, log, &recorder) {}

    [[nodiscard]] static mu::session::SessionOptions Options() {
        mu::session::SessionOptions options;
        options.account = std::string(kAccount);
        options.password = std::string(kPassword);
        return options;
    }

    // Steps 2 and 3 of a frame.
    void Frame(double now) {
        client.TakeEvents(Seconds{now});
        client.AdvanceClock(Seconds{now});
    }

    // The start of a tick; gives how many events it took.
    std::size_t Tick(double now) {
        const std::size_t pending = client.PendingEvents();
        client.ApplyEvents(Seconds{now});
        return pending;
    }

    // From the Start to the answer of the login, one answer of the server for each frame, 0.1 s apart.
    void WalkToTheLogin(std::uint8_t loginResult) {
        client.Start(Seconds{0.0});
        const std::vector<Bytes> answers{Hello(), ServerList(), ConnectionInfo(), GameServerEntered(),
                                         LoginResponse(loginResult)};
        for (std::size_t step = 0; step < answers.size(); ++step) {
            if (step == 0 || step == 3) {
                transport.Connected();
            }
            transport.Message(answers[step]);
            Frame(0.1 * static_cast<double>(step + 1));
        }
    }

    // On to the data of the character: the entry in the world is in the frame at 0.7 s.
    void WalkToTheWorld() {
        WalkToTheLogin(1);
        transport.Message(CharacterList({"test0Dk"}));
        Frame(0.6);
        transport.Message(CharacterInformation());
        Frame(0.7);
        REQUIRE(server.State() == mu::session::SessionState::InWorld);
    }

    void Finish(double now, EndReason reason) { recorder.Finish(client.View(), Seconds{now}, reason); }

    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    Recorder recorder;
    test_transport::ScriptedTransport transport;
    mu::online::LiveServer server;
    mu::online::OnlineClient client{server, log, std::nullopt};
};

// A view of the world fed with events alone, for the recorder without a server.
struct View {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    world::WorldView view{log};
};

[[nodiscard]] world::ObjectEntered Monster(std::uint16_t id) {
    world::ObjectEntered entered;
    entered.id = world::ObjectId{id};
    entered.kind = world::ObjectKind::NpcOrMonster;
    entered.position = {135, 124};
    entered.target = {135, 124};
    entered.typeNumber = 14;
    return entered;
}

} // namespace

TEST_CASE(
    "[0002-CA-04] the live server records from the entry in the world each message, tick and checkpoint") {
    LiveRecording run;
    run.WalkToTheWorld();
    const std::size_t first = run.Tick(0.7);

    run.transport.Message(HeroEntered());
    run.transport.Message(NpcCameIntoView());
    run.Frame(1.0);
    // A frame without messages and without a tick, then the tick.
    run.Frame(1.02);
    const std::size_t second = run.Tick(1.04);

    run.transport.Message(CurrentStats());
    run.Frame(5.6);
    const std::size_t third = run.Tick(5.6);

    // The first tick from 5 s in the world on, which entered at 0.7 s, has the first checkpoint.
    run.transport.Message(HeroWalked());
    run.Frame(5.8);
    const std::size_t fourth = run.Tick(5.8);
    const Checkpoint at5 = CheckpointOf(run.client.View(), 5'100'000, 5);
    // A tick with nothing in the queue is not recorded.
    CHECK(run.Tick(6.0) == 0);

    run.transport.Message(NpcLeftTheView());
    run.Frame(7.0);
    const std::size_t fifth = run.Tick(7.0);
    run.Finish(7.5, EndReason::Closed);

    const auto count = [](std::size_t events) { return static_cast<std::uint32_t>(events); };
    const Bytes data = CharacterInformation();
    const Bytes hero = HeroEntered();
    const Bytes npc = NpcCameIntoView();
    const Bytes stats = CurrentStats();
    const Bytes walk = HeroWalked();
    const Bytes left = NpcLeftTheView();
    CheckSame(RecordsOf(run.recorder.Bytes()),
              Lines({
                  EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                  WorldMessage{.time = 0, .epoch = kGameServer, .bytes = data},
                  TickMark{.time = 0, .events = count(first)},
                  WorldMessage{.time = 300'000, .epoch = kGameServer, .bytes = hero},
                  WorldMessage{.time = 300'000, .epoch = kGameServer, .bytes = npc},
                  TickMark{.time = 340'000, .events = count(second)},
                  WorldMessage{.time = 4'900'000, .epoch = kGameServer, .bytes = stats},
                  TickMark{.time = 4'900'000, .events = count(third)},
                  WorldMessage{.time = 5'100'000, .epoch = kGameServer, .bytes = walk},
                  TickMark{.time = 5'100'000, .events = count(fourth)},
                  at5,
                  WorldMessage{.time = 6'300'000, .epoch = kGameServer, .bytes = left},
                  TickMark{.time = 6'300'000, .events = count(fifth)},
                  CheckpointOf(run.client.View(), 6'800'000, 6),
                  RecordingEnd{.time = 6'800'000, .reason = EndReason::Closed},
              }));
    CHECK(first > 0);
    CHECK(fifth > 0);
    CHECK(run.recorder.Messages() == 6);
    CHECK(run.recorder.Ticks() == 5);
    CHECK(run.recorder.Checkpoints() == 2);
    CHECK(run.recorder.Ended());
}

TEST_CASE(
    "[0002-CA-04] the recording has neither the account, nor the password, nor anything the client sent") {
    LiveRecording run;
    run.WalkToTheWorld();
    run.Tick(0.7);
    run.transport.Message(HeroEntered());
    run.Frame(1.0);
    run.Tick(1.0);
    run.Finish(1.5, EndReason::Closed);
    const std::span<const std::byte> file = run.recorder.Bytes();

    CHECK_FALSE(Contains(file, kAccount));
    CHECK_FALSE(Contains(file, kPassword));
    // The login carries them through Xor3, so the search for each message the client sent finds them too.
    REQUIRE(run.transport.sent.size() >= 3);
    for (const Bytes& sent : run.transport.sent) {
        CHECK_MESSAGE(!Contains(file, sent), Hex(sent));
    }
    // Nothing from before the world.
    for (const Bytes& before :
         {ServerList(), ConnectionInfo(), GameServerEntered(), CharacterList({"test0Dk"})}) {
        CHECK_MESSAGE(!Contains(file, before), Hex(before));
    }
}

TEST_CASE("[0002-CA-04] control: the search finds the login with its Xor3 when it is in a recording") {
    LiveRecording run;
    run.WalkToTheLogin(1);
    const auto login = std::ranges::find_if(run.transport.sent, [](const Bytes& message) {
        return message.size() > 4 && message[2] == std::byte{0xF1} && message[3] == std::byte{0x01};
    });
    REQUIRE(login != run.transport.sent.end());
    // The plain password is not in the login, which is why the search looks for the messages themselves.
    CHECK_FALSE(Contains(*login, kPassword));

    View view;
    Recorder leaking = MakeRecorder(view.log);
    leaking.EnterWorld(Seconds{1.0}, kGameServer, kHero);
    leaking.NoteMessage(Seconds{1.0}, kGameServer, *login);
    CHECK(Contains(leaking.Bytes(), *login));
}

TEST_CASE("[0002-CA-04] a session that never reached the world records only the header and the end") {
    LiveRecording run;
    run.WalkToTheLogin(0); // invalid password (section 6.7)
    REQUIRE(run.server.Status().state == mu::online::ServerState::Failed);
    run.Finish(0.6, EndReason::SessionFailed);
    CheckSame(RecordsOf(run.recorder.Bytes()),
              Lines({RecordingEnd{.time = 0, .reason = EndReason::SessionFailed}}));
    CHECK(run.recorder.Bytes().size() == kHeaderSize + 5 + 9);
    CHECK(run.recorder.Messages() == 0);
}

TEST_CASE("[0002-CA-04] the recorder takes nothing before the entry in the world") {
    View view;
    Recorder recorder = MakeRecorder(view.log);
    const Bytes message = HeroEntered();
    recorder.NoteEpoch(Seconds{0.5}, SessionEpoch{2});
    recorder.NoteMessage(Seconds{0.5}, kGameServer, message);
    recorder.NoteTick(view.view, 3, Seconds{0.5});
    CHECK(recorder.Bytes().size() == kHeaderSize);
    CHECK(recorder.Messages() == 0);
    CHECK(recorder.Ticks() == 0);

    // The entry counts once, and the clock starts with it.
    recorder.EnterWorld(Seconds{2.0}, kGameServer, kHero);
    recorder.EnterWorld(Seconds{3.0}, SessionEpoch{3}, world::ObjectId{0x0300});
    recorder.Finish(view.view, Seconds{2.5}, EndReason::Closed);
    CheckSame(RecordsOf(recorder.Bytes()),
              Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                     RecordingEnd{.time = 500'000, .reason = EndReason::Closed}}));
}

TEST_CASE("[0002-CA-04] the recorder puts a checkpoint at the first tick from each 5 s in the world on") {
    View view;
    Recorder recorder = MakeRecorder(view.log);
    const Bytes message = NpcCameIntoView();
    recorder.EnterWorld(Seconds{2.0}, kGameServer, kHero);
    const auto tick = [&](double now, std::uint16_t monster) {
        recorder.NoteMessage(Seconds{now}, kGameServer, message);
        view.view.Apply(Monster(monster));
        recorder.NoteTick(view.view, 1, Seconds{now});
    };

    tick(6.9, 1);
    tick(7.0, 2);
    const Checkpoint at7 = CheckpointOf(view.view, 5'000'000, 2);
    tick(11.9, 3);
    tick(12.5, 4);
    const Checkpoint at12 = CheckpointOf(view.view, 10'500'000, 4);
    // After a long time without a tick, one checkpoint, and the next at the grid of 5 s: 32 s.
    tick(30.0, 5);
    const Checkpoint at30 = CheckpointOf(view.view, 28'000'000, 5);
    tick(31.9, 6);
    tick(32.0, 7);
    const Checkpoint at32 = CheckpointOf(view.view, 30'000'000, 7);
    // A time before the last one is recorded as the last one: the time of a recording never goes back.
    recorder.NoteMessage(Seconds{31.5}, kGameServer, message);
    recorder.Finish(view.view, Seconds{33.0}, EndReason::Closed);

    const auto entry = EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero};
    const auto at = [&](std::uint64_t time) {
        return WorldMessage{.time = time, .epoch = kGameServer, .bytes = message};
    };
    const auto mark = [](std::uint64_t time) { return TickMark{.time = time, .events = 1}; };
    CheckSame(RecordsOf(recorder.Bytes()),
              Lines({entry,
                     at(4'900'000),
                     mark(4'900'000),
                     at(5'000'000),
                     mark(5'000'000),
                     at7,
                     at(9'900'000),
                     mark(9'900'000),
                     at(10'500'000),
                     mark(10'500'000),
                     at12,
                     at(28'000'000),
                     mark(28'000'000),
                     at30,
                     at(29'900'000),
                     mark(29'900'000),
                     at(30'000'000),
                     mark(30'000'000),
                     at32,
                     at(30'000'000),
                     RecordingEnd{.time = 31'000'000, .reason = EndReason::Closed}}));
    CHECK(recorder.Checkpoints() == 4);
    // Each checkpoint saw another view.
    CHECK(at7.digest != at12.digest);
    CHECK(at12.digest != at30.digest);
    CHECK(at30.digest != at32.digest);
}

TEST_CASE("[0002-CA-04] the recorder records a change of epoch, and not the same epoch again") {
    View view;
    Recorder recorder = MakeRecorder(view.log);
    recorder.EnterWorld(Seconds{1.0}, kGameServer, kHero);
    recorder.NoteEpoch(Seconds{1.5}, kGameServer);
    recorder.NoteEpoch(Seconds{2.0}, SessionEpoch{3});
    recorder.NoteEpoch(Seconds{2.5}, SessionEpoch{3});
    recorder.Finish(view.view, Seconds{3.0}, EndReason::Closed);
    CheckSame(RecordsOf(recorder.Bytes()),
              Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                     EpochChanged{.time = 1'000'000, .epoch = SessionEpoch{3}},
                     RecordingEnd{.time = 2'000'000, .reason = EndReason::Closed}}));
}

TEST_CASE("[0002-CA-04] the end has its checkpoint only right after a tick that has none, and comes once") {
    const Bytes message = HeroEntered();

    SUBCASE("after a tick") {
        View view;
        Recorder recorder = MakeRecorder(view.log);
        recorder.EnterWorld(Seconds{1.0}, kGameServer, kHero);
        recorder.NoteMessage(Seconds{1.0}, kGameServer, message);
        recorder.NoteTick(view.view, 2, Seconds{1.0});
        recorder.Finish(view.view, Seconds{2.0}, EndReason::SessionFailed);
        recorder.Finish(view.view, Seconds{3.0}, EndReason::Closed);
        CheckSame(RecordsOf(recorder.Bytes()),
                  Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                         WorldMessage{.time = 0, .epoch = kGameServer, .bytes = message},
                         TickMark{.time = 0, .events = 2}, CheckpointOf(view.view, 1'000'000, 1),
                         RecordingEnd{.time = 1'000'000, .reason = EndReason::SessionFailed}}));
    }
    SUBCASE("after a message, which waits for a tick") {
        View view;
        Recorder recorder = MakeRecorder(view.log);
        recorder.EnterWorld(Seconds{1.0}, kGameServer, kHero);
        recorder.NoteTick(view.view, 2, Seconds{1.0});
        recorder.NoteMessage(Seconds{1.5}, kGameServer, message);
        recorder.Finish(view.view, Seconds{2.0}, EndReason::Closed);
        CheckSame(RecordsOf(recorder.Bytes()),
                  Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                         TickMark{.time = 0, .events = 2},
                         WorldMessage{.time = 500'000, .epoch = kGameServer, .bytes = message},
                         RecordingEnd{.time = 1'000'000, .reason = EndReason::Closed}}));
    }
    SUBCASE("after a tick that already has its checkpoint") {
        View view;
        Recorder recorder = MakeRecorder(view.log);
        recorder.EnterWorld(Seconds{1.0}, kGameServer, kHero);
        recorder.NoteTick(view.view, 2, Seconds{6.0});
        recorder.Finish(view.view, Seconds{6.5}, EndReason::Closed);
        CheckSame(RecordsOf(recorder.Bytes()),
                  Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                         TickMark{.time = 5'000'000, .events = 2}, CheckpointOf(view.view, 5'000'000, 0),
                         RecordingEnd{.time = 5'500'000, .reason = EndReason::Closed}}));
    }
}

TEST_CASE(
    "[0002-CA-04] a recording that fills its room stops with one warning, stays valid and ends as full") {
    View view;
    // The header, the entry (19 bytes), two messages of 10 bytes (27 each) and the end (14).
    constexpr std::size_t kSmallRoom = kHeaderSize + 19 + (2 * 27) + 14;
    Recorder recorder = MakeRecorder(view.log, kSmallRoom);
    const Bytes message = Make({0xC1, 0x0A, 0xD4, 0x02, 0x00, 0x91, 0x7B, 0x90, 0x7E, 0x30});
    recorder.EnterWorld(Seconds{1.0}, kGameServer, kHero);
    recorder.NoteMessage(Seconds{1.0}, kGameServer, message);
    recorder.NoteMessage(Seconds{2.0}, kGameServer, message);
    CHECK_FALSE(recorder.Full());
    CHECK(WarningsOf(view.sink) == 0);

    recorder.NoteMessage(Seconds{3.5}, kGameServer, message);
    CHECK(recorder.Full());
    REQUIRE(WarningsOf(view.sink) == 1);
    CHECK(view.sink.Entries().back().message == "the recording filled its room after 2 messages, at 2 s in "
                                                "the world: it stops here, and its end says it "
                                                "is full");
    // Nothing else enters, and the warning is not repeated.
    recorder.NoteMessage(Seconds{4.0}, kGameServer, message);
    recorder.NoteTick(view.view, 3, Seconds{4.0});
    recorder.NoteEpoch(Seconds{4.0}, SessionEpoch{3});
    CHECK(WarningsOf(view.sink) == 1);
    CHECK(recorder.Messages() == 2);
    CHECK(recorder.Ticks() == 0);

    recorder.Finish(view.view, Seconds{5.0}, EndReason::Closed);
    CHECK(recorder.Bytes().size() == kSmallRoom);
    CheckSame(RecordsOf(recorder.Bytes()),
              Lines({EnteredWorld{.time = 0, .epoch = kGameServer, .hero = kHero},
                     WorldMessage{.time = 0, .epoch = kGameServer, .bytes = message},
                     WorldMessage{.time = 1'000'000, .epoch = kGameServer, .bytes = message},
                     RecordingEnd{.time = 4'000'000, .reason = EndReason::Full}}));
}

TEST_CASE(
    "[0002-CA-04] the recorder refuses a room too small for the header and the end, as the writer does") {
    View view;
    const auto recorder = Recorder::Create(TestHeader(), kHeaderSize + 13, view.log);
    REQUIRE_FALSE(recorder.has_value());
    CHECK(recorder.error().message ==
          "the room of 51 bytes is too small for the header and the end, which take 52");
}
