// The online client in the steps of the frame (specs/0001-rede-ate-o-mundo/spec.md), with the live server
// (ADR 0035): the session fed through a transport in memory, the entry in the world as soon as the data of
// the character comes, the messages of the world translated in step 2 and applied only at the start of a
// tick, and the end of the client when the session fails or stayed as long as asked. The messages come from
// SessionMessages.hpp, with their sources. Then the client with a server of test, which gives the events
// themselves (CA-01 of slice 2), and the tick that drops the events of an older epoch (CA-02 of slice 2).
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Online/LiveServer.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Online/Transport.hpp"
#include "Mu/Protocol/Dialect/World.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "ScriptedTransport.hpp"
#include "SessionMessages.hpp"

namespace {

using mu::online::OnlineClient;
using mu::online::OnlineStatus;
using mu::online::ServerState;
using mu::session::SessionEpoch;
using mu::session::SessionState;
using nexum::time::Seconds;
using test_transport::ScriptedTransport;
using namespace session_messages;

[[nodiscard]] mu::session::SessionOptions TestOptions() {
    mu::session::SessionOptions options;
    options.connectServerHost = "127.0.0.1";
    options.connectServerPort = 44406;
    options.account = "test0";
    options.password = "test0";
    return options;
}

// A client with the live server, its transport and its log, and the steps of the way to the world.
struct Run {
    explicit Run(std::optional<Seconds> stay = std::nullopt)
        : server(TestOptions(), transport, log), client(server, log, stay) {}

    // One frame without ticks: steps 2 and 3.
    void Frame(double now) {
        client.TakeEvents(Seconds{now});
        client.AdvanceClock(Seconds{now});
    }

    // From the Start to the data of the character, one answer of the server for each frame, 0.1 s apart. The
    // last frame is at 0.7 s.
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
    }

    [[nodiscard]] bool LogHas(std::string_view text) const {
        for (const auto& entry : sink.Entries()) {
            if (entry.message.find(text) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ScriptedTransport transport;
    mu::online::LiveServer server;
    OnlineClient client;
};

// What a server of test was told after a tick.
struct AppliedReport {
    std::size_t count = 0;
    double now = 0.0;
    std::size_t objectsInView = 0;

    friend bool operator==(const AppliedReport&, const AppliedReport&) = default;
};

// A server of test, without session, translation or network: each TakeEvents gives the next batch of events
// that the test wrote, with the epoch of the server at that moment, and the test sets the state and the
// epoch. It keeps what each tick told it.
class ScriptedServer final : public mu::online::ServerAdapter {
public:
    void Start(Seconds now) override { started = now.count(); }

    void TakeEvents(Seconds /*now*/, mu::world::EventQueue& events) override {
        ++takes;
        if (m_next < batches.size()) {
            for (const mu::world::WorldEvent& event : batches[m_next]) {
                events.Push(epoch, event);
            }
            ++m_next;
        }
    }

    void AdvanceClock(Seconds /*now*/) override { ++clocks; }

    void EventsApplied(const mu::world::WorldView& view, std::size_t count, Seconds now) override {
        applied.push_back({.count = count, .now = now.count(), .objectsInView = view.Objects().size()});
    }

    [[nodiscard]] SessionEpoch CurrentEpoch() const noexcept override { return epoch; }

    [[nodiscard]] mu::online::ServerStatus Status() const noexcept override {
        return {.state = state, .reason = reason};
    }

    [[nodiscard]] std::string Summary(Seconds /*now*/) const override { return "a server of test"; }

    std::vector<std::vector<mu::world::WorldEvent>> batches;
    SessionEpoch epoch{1};
    ServerState state = ServerState::Connecting;
    std::string reason;
    std::vector<AppliedReport> applied;
    std::optional<double> started;
    int takes = 0;
    int clocks = 0;

private:
    std::size_t m_next = 0;
};

[[nodiscard]] mu::world::ObjectEntered EnteredAt(std::uint16_t id, mu::world::Cell cell) {
    mu::world::ObjectEntered entered;
    entered.id = mu::world::ObjectId{id};
    entered.kind = mu::world::ObjectKind::NpcOrMonster;
    entered.position = cell;
    entered.target = cell;
    entered.typeNumber = 249;
    return entered;
}

// The lines of the log about events dropped for their epoch.
[[nodiscard]] std::vector<std::string> DropLines(const nexum::test_support::MemoryLogSink& sink) {
    std::vector<std::string> lines;
    for (const auto& entry : sink.Entries()) {
        if (entry.message.find("dropped") != std::string::npos) {
            CHECK(entry.level == nexum::debug::LogLevel::Info);
            CHECK(entry.category == "online");
            lines.push_back(entry.message);
        }
    }
    return lines;
}

} // namespace

TEST_CASE(
    "[0001-CA-07] the online client takes the network in step 2 and enters the world with the character") {
    Run run;
    run.client.Start(Seconds{0.0});
    REQUIRE(run.transport.opened.size() == 1);
    CHECK(run.transport.opened[0].port == 44406);
    CHECK(run.server.State() == SessionState::ConnectServer);

    // Nothing reaches the session before step 2 of a frame.
    run.transport.Connected();
    run.transport.Message(Hello());
    run.client.AdvanceClock(Seconds{0.05});
    CHECK(run.transport.sent.empty());
    run.Frame(0.1);
    REQUIRE(run.transport.sent.size() == 1);
    CHECK(run.transport.sent[0] == Make({0xC1, 0x04, 0xF4, 0x06}));

    run.transport.Message(ServerList());
    run.Frame(0.2);
    run.transport.Message(ConnectionInfo());
    run.Frame(0.3);
    REQUIRE(run.transport.opened.size() == 2);
    CHECK(run.transport.opened[1].port == 45902);
    CHECK(run.transport.closes == 1);

    run.transport.Connected();
    run.transport.Message(GameServerEntered());
    run.Frame(0.4);
    run.transport.Message(LoginResponse(1));
    run.Frame(0.5);
    run.transport.Message(CharacterList({"test0Dk"}));
    run.Frame(0.6);
    // The character is chosen, and its data is awaited.
    CHECK(run.server.State() == SessionState::Loading);

    // The map is ready as soon as the data of the character comes: there is no map to load in this slice.
    run.transport.Message(CharacterInformation());
    run.Frame(0.7);
    CHECK(run.server.State() == SessionState::InWorld);
    CHECK(run.LogHas("in the world"));
}

TEST_CASE(
    "[0001-CA-09] the messages of the world become events in step 2 and change the view only in the tick") {
    Run run;
    run.WalkToTheWorld();
    REQUIRE(run.server.State() == SessionState::InWorld);

    // The data of the character was translated, and waits for the tick.
    CHECK(run.client.PendingEvents() > 0);
    CHECK_FALSE(run.client.View().HeroState().loaded);

    run.client.ApplyEvents(Seconds{0.7});
    CHECK(run.client.PendingEvents() == 0);
    // The events have the epoch of the Game Server, which is the current one: none is dropped.
    CHECK(run.server.CurrentEpoch() == SessionEpoch{2});
    CHECK(run.client.DroppedEvents() == 0);
    const auto& hero = run.client.View().HeroState();
    REQUIRE(hero.loaded);
    // The hero has the id that the Game Server gave (GameServerEntered), at the place of its data.
    CHECK(hero.id == mu::world::ObjectId{0x0200});
    CHECK(hero.position == mu::world::Cell{.x = 145, .y = 123});

    // A message of the world in a later frame waits for step 2, and then for the next tick.
    run.transport.Message(HeroEntered());
    CHECK(run.client.PendingEvents() == 0);
    run.Frame(0.8);
    const std::size_t pending = run.client.PendingEvents();
    CHECK(pending > 0);
    run.client.AdvanceClock(Seconds{0.9});
    CHECK(run.client.PendingEvents() == pending);
    run.client.ApplyEvents(Seconds{0.9});
    CHECK(run.client.PendingEvents() == 0);
}

TEST_CASE("[0001-CA-08] the online client sends the ping in step 3 and counts it in the summary") {
    Run run;
    run.WalkToTheWorld();
    run.client.ApplyEvents(Seconds{0.7});
    const auto pings = [&run] {
        return std::ranges::count_if(run.transport.sent, [](const Bytes& message) {
            return mu::protocol::dialect::Ping::View::Read(message).has_value();
        });
    };

    // The ping runs from the entry in the Game Server, at 0.4 s: the first one is due at 20.4 s.
    run.client.AdvanceClock(Seconds{20.3});
    CHECK(pings() == 0);
    run.client.AdvanceClock(Seconds{20.5});
    CHECK(pings() == 1);

    // In the world since 0.7 s; the seconds are whole.
    const std::string objects = std::to_string(run.client.View().Objects().size());
    CHECK(run.client.Summary(Seconds{31.0}) ==
          "the session ends: 30 s in the world, pings sent: 1, objects in view: " + objects);
}

TEST_CASE("the online client stays in the world as long as asked, and runs until then") {
    Run run(Seconds{60.0});
    CHECK(run.client.Status(Seconds{0.0}) == OnlineStatus::Running);
    run.WalkToTheWorld();
    // The time counts from the entry in the world, at 0.7 s.
    CHECK(run.client.Status(Seconds{60.6}) == OnlineStatus::Running);
    CHECK(run.client.Status(Seconds{60.8}) == OnlineStatus::StayedLongEnough);

    Run forever;
    forever.WalkToTheWorld();
    CHECK(forever.client.Status(Seconds{86'400.0}) == OnlineStatus::Running);
}

TEST_CASE("[0001-CA-07] the online client fails with the session, and its summary says it never entered") {
    Run run;
    run.client.Start(Seconds{0.0});
    run.transport.Connected();
    run.transport.Message(Hello());
    run.transport.Message(ServerList());
    run.transport.Message(ConnectionInfo());
    run.Frame(0.1);
    run.transport.Connected();
    run.transport.Message(GameServerEntered());
    run.transport.Message(LoginResponse(0)); // invalid password (section 6.7)
    run.Frame(0.2);

    CHECK(run.client.Status(Seconds{0.2}) == OnlineStatus::Failed);
    CHECK(run.LogHas("the login was refused: invalid password"));
    CHECK(run.client.Summary(Seconds{0.2}) == "the session ends: never in the world, pings sent: 0");
    // The live server fails with the reason of the session.
    CHECK(run.server.Status().state == ServerState::Failed);
    CHECK(run.server.Status().reason == "the login was refused: invalid password (code 0)");
}

TEST_CASE("[0001-CA-07] a connection that closes ends the online client with the reason") {
    Run run;
    run.client.Start(Seconds{0.0});
    run.transport.Closed("connection refused");
    run.Frame(0.1);
    CHECK(run.client.Status(Seconds{0.1}) == OnlineStatus::Failed);
    CHECK(run.LogHas("the connection to the Connect Server closed: connection refused"));
    CHECK(run.server.Status().reason == "the connection to the Connect Server closed: connection refused");
}

TEST_CASE("[0002-CA-01] the live server is connecting until the world, then in the world, with no reason") {
    Run run;
    CHECK(run.server.Status().state == ServerState::Connecting);
    run.client.Start(Seconds{0.0});
    CHECK(run.server.Status().state == ServerState::Connecting);
    run.WalkToTheWorld();
    CHECK(run.server.Status().state == ServerState::InWorld);
    CHECK(run.server.Status().reason.empty());
    CHECK(run.server.Summary(Seconds{1.0}) == "pings sent: 0");
}

TEST_CASE(
    "[0002-CA-01] the online client applies the events of any server in their order, only in the tick") {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ScriptedServer server;
    // The order matters: the correction and the leaving come after the entries, and either one first would
    // be about an object not in view, which the view ignores.
    server.batches = {
        {EnteredAt(0x21, {.x = 10, .y = 10}),
         mu::world::PositionCorrected{.id = {0x21}, .position = {12, 12}},
         EnteredAt(0x22, {.x = 30, .y = 30}), mu::world::ObjectLeft{.id = {0x22}}},
        {mu::world::ObjectLeft{.id = {0x21}}},
    };
    OnlineClient client(server, log, std::nullopt);
    client.Start(Seconds{0.0});
    CHECK(server.started == 0.0);

    // Step 2 takes the events and step 3 runs the clock, but only the tick applies them.
    client.TakeEvents(Seconds{0.1});
    client.AdvanceClock(Seconds{0.1});
    CHECK(server.clocks == 1);
    CHECK(client.PendingEvents() == 4);
    CHECK(client.View().Objects().empty());
    CHECK(server.applied.empty());

    client.ApplyEvents(Seconds{0.1});
    CHECK(client.PendingEvents() == 0);
    REQUIRE(client.View().Objects().size() == 1);
    const mu::world::WorldObject* object = client.View().Find(mu::world::ObjectId{0x21});
    REQUIRE(object != nullptr);
    CHECK(object->position == mu::world::Cell{.x = 12, .y = 12});
    CHECK(client.View().IgnoredEvents() == 0);
    CHECK(server.applied == std::vector<AppliedReport>{{.count = 4, .now = 0.1, .objectsInView = 1}});

    // A second tick of the same frame has no events, and the server hears nothing of it.
    client.ApplyEvents(Seconds{0.1});
    CHECK(server.applied.size() == 1);

    client.TakeEvents(Seconds{0.2});
    client.ApplyEvents(Seconds{0.2});
    CHECK(client.View().Objects().empty());
    CHECK(server.applied == std::vector<AppliedReport>{{.count = 4, .now = 0.1, .objectsInView = 1},
                                                       {.count = 1, .now = 0.2, .objectsInView = 0}});

    // A frame whose server has nothing new.
    client.TakeEvents(Seconds{0.3});
    client.ApplyEvents(Seconds{0.3});
    CHECK(server.takes == 3);
    CHECK(server.applied.size() == 2);
}

TEST_CASE("[0002-CA-01] the status of the online client follows the state of the server, in any server") {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ScriptedServer server;
    server.batches = {{}, {EnteredAt(0x21, {.x = 10, .y = 10})}};
    OnlineClient client(server, log, Seconds{10.0});
    client.Start(Seconds{0.0});

    client.TakeEvents(Seconds{0.1});
    CHECK(client.Status(Seconds{0.1}) == OnlineStatus::Running);
    CHECK(client.Summary(Seconds{0.1}) == "the session ends: never in the world, a server of test");

    // The time in the world counts from the frame whose step 2 saw the server in the world.
    server.state = ServerState::InWorld;
    client.TakeEvents(Seconds{0.5});
    client.ApplyEvents(Seconds{0.5});
    CHECK(client.Status(Seconds{10.4}) == OnlineStatus::Running);
    CHECK(client.Status(Seconds{10.5}) == OnlineStatus::StayedLongEnough);
    CHECK(client.Summary(Seconds{2.9}) ==
          "the session ends: 2 s in the world, a server of test, objects in view: 1");

    server.state = ServerState::Ended;
    CHECK(client.Status(Seconds{0.6}) == OnlineStatus::ServerEnded);
    server.state = ServerState::Failed;
    server.reason = "the recording ended early";
    CHECK(client.Status(Seconds{0.6}) == OnlineStatus::Failed);
}

TEST_CASE(
    "[0002-CA-02] the tick drops and counts the events of an older epoch, and logs it once in the tick") {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ScriptedServer server;
    server.batches = {
        {EnteredAt(0x21, {.x = 10, .y = 10}), EnteredAt(0x22, {.x = 20, .y = 20})},
        {EnteredAt(0x23, {.x = 30, .y = 30})},
        {EnteredAt(0x24, {.x = 40, .y = 40})},
        {EnteredAt(0x25, {.x = 50, .y = 50})},
    };
    OnlineClient client(server, log, std::nullopt);
    client.Start(Seconds{0.0});

    // Two events are taken in the epoch 1, and the epoch changes before the tick; one more comes in the
    // epoch 2, for the same tick.
    client.TakeEvents(Seconds{0.1});
    server.epoch = SessionEpoch{2};
    client.TakeEvents(Seconds{0.2});
    REQUIRE(client.PendingEvents() == 3);
    client.ApplyEvents(Seconds{0.2});

    // The old ones never reach the view, which ignores nothing; the new one does.
    CHECK(client.PendingEvents() == 0);
    CHECK(client.View().Find(mu::world::ObjectId{0x21}) == nullptr);
    CHECK(client.View().Find(mu::world::ObjectId{0x22}) == nullptr);
    CHECK(client.View().Find(mu::world::ObjectId{0x23}) != nullptr);
    CHECK(client.View().Objects().size() == 1);
    CHECK(client.View().IgnoredEvents() == 0);
    CHECK(client.DroppedEvents() == 2);
    // The server hears of every event the tick took, applied or dropped (the tick of the .mucap).
    CHECK(server.applied == std::vector<AppliedReport>{{.count = 3, .now = 0.2, .objectsInView = 1}});
    CHECK(DropLines(sink) == std::vector<std::string>{"the tick dropped 2 events of an epoch older than 2"});

    // A tick with only events of the current epoch drops nothing and says nothing.
    client.TakeEvents(Seconds{0.3});
    client.ApplyEvents(Seconds{0.3});
    CHECK(client.View().Find(mu::world::ObjectId{0x24}) != nullptr);
    CHECK(client.DroppedEvents() == 2);
    CHECK(DropLines(sink).size() == 1);

    // A tick whose events are all old applies none, still tells the server how many it took, and logs once
    // more.
    client.TakeEvents(Seconds{0.4});
    server.epoch = SessionEpoch{3};
    client.ApplyEvents(Seconds{0.4});
    CHECK(client.View().Find(mu::world::ObjectId{0x25}) == nullptr);
    CHECK(client.View().Objects().size() == 2);
    CHECK(client.DroppedEvents() == 3);
    CHECK(server.applied.size() == 3);
    CHECK(server.applied.back() == AppliedReport{.count = 1, .now = 0.4, .objectsInView = 2});
    CHECK(DropLines(sink) == std::vector<std::string>{"the tick dropped 2 events of an epoch older than 2",
                                                      "the tick dropped 1 event of an epoch older than 3"});
}

TEST_CASE("[0002-CA-02] many old events in one tick give a single line of the log") {
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log{sink};
    ScriptedServer server;
    std::vector<mu::world::WorldEvent> many;
    for (std::uint16_t id = 0x30; id < 0x30 + 50; ++id) {
        many.emplace_back(EnteredAt(id, {.x = 10, .y = 10}));
    }
    server.batches = {many};
    OnlineClient client(server, log, std::nullopt);
    client.Start(Seconds{0.0});
    client.TakeEvents(Seconds{0.1});
    server.epoch = SessionEpoch{2};
    client.ApplyEvents(Seconds{0.1});
    CHECK(client.View().Objects().empty());
    CHECK(client.DroppedEvents() == 50);
    CHECK(DropLines(sink) == std::vector<std::string>{"the tick dropped 50 events of an epoch older than 2"});
}
