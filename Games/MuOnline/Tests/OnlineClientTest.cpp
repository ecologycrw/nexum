// The online client in the steps of the frame (specs/0001-rede-ate-o-mundo/spec.md): the session fed through
// a transport in memory, the entry in the world as soon as the data of the character comes, the messages of
// the world translated in step 2 and applied only at the start of a tick, and the end of the client when the
// session fails or stayed as long as asked. The messages come from SessionMessages.hpp, with their sources.
#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Online/Transport.hpp"
#include "Mu/Protocol/Dialect/World.hpp"
#include "Mu/Session/Session.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "SessionMessages.hpp"

namespace {

using mu::online::OnlineClient;
using mu::online::OnlineStatus;
using mu::session::Endpoint;
using mu::session::Session;
using mu::session::SessionEpoch;
using mu::session::SessionState;
using nexum::net::EventKind;
using nexum::time::Seconds;
using namespace session_messages;

// A transport with the events in memory: the test queues them, each with the epoch of the connection open
// when it was queued, and the next Deliver hands them over.
class ScriptedTransport final : public mu::online::Transport {
public:
    void Open(const Endpoint& endpoint) override {
        opened.push_back(endpoint);
        m_epoch = endpoint.epoch;
    }

    nexum::core::Result<void> Send(std::span<const std::byte> message) override {
        sent.emplace_back(message.begin(), message.end());
        return {};
    }

    void Close() override { ++closes; }

    std::size_t Deliver(Session& session, Seconds now) override {
        std::vector<Queued> queued = std::exchange(m_queued, {});
        for (const Queued& event : queued) {
            session.Handle(event.epoch,
                           {.kind = event.kind, .message = event.message, .reason = event.reason}, now);
        }
        ++deliveries;
        return queued.size();
    }

    void Connected() { m_queued.push_back({m_epoch, EventKind::Connected, {}, {}}); }
    void Message(Bytes message) { m_queued.push_back({m_epoch, EventKind::Message, std::move(message), {}}); }
    void Closed(std::string reason) {
        m_queued.push_back({m_epoch, EventKind::Closed, {}, std::move(reason)});
    }

    std::vector<Endpoint> opened;
    std::vector<Bytes> sent;
    int closes = 0;
    int deliveries = 0;

private:
    struct Queued {
        SessionEpoch epoch;
        EventKind kind = EventKind::Closed;
        Bytes message;
        std::string reason;
    };

    std::vector<Queued> m_queued;
    SessionEpoch m_epoch;
};

[[nodiscard]] mu::session::SessionOptions TestOptions() {
    mu::session::SessionOptions options;
    options.connectServerHost = "127.0.0.1";
    options.connectServerPort = 44406;
    options.account = "test0";
    options.password = "test0";
    return options;
}

// A client with its transport and its log, and the steps of the way to the world.
struct Run {
    explicit Run(std::optional<Seconds> stay = std::nullopt) : client(TestOptions(), transport, log, stay) {}

    // One frame without ticks: steps 2 and 3.
    void Frame(double now) {
        client.TakeNetwork(Seconds{now});
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
    OnlineClient client;
};

} // namespace

TEST_CASE(
    "[0001-CA-07] the online client takes the network in step 2 and enters the world with the character") {
    Run run;
    run.client.Start(Seconds{0.0});
    REQUIRE(run.transport.opened.size() == 1);
    CHECK(run.transport.opened[0].port == 44406);
    CHECK(run.client.State() == SessionState::ConnectServer);

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
    CHECK(run.client.State() == SessionState::Loading);

    // The map is ready as soon as the data of the character comes: there is no map to load in this slice.
    run.transport.Message(CharacterInformation());
    run.Frame(0.7);
    CHECK(run.client.State() == SessionState::InWorld);
    CHECK(run.LogHas("in the world"));
}

TEST_CASE(
    "[0001-CA-09] the messages of the world become events in step 2 and change the view only in the tick") {
    Run run;
    run.WalkToTheWorld();
    REQUIRE(run.client.State() == SessionState::InWorld);

    // The data of the character was translated, and waits for the tick.
    CHECK(run.client.PendingEvents() > 0);
    CHECK_FALSE(run.client.View().HeroState().loaded);

    run.client.ApplyEvents();
    CHECK(run.client.PendingEvents() == 0);
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
    run.client.ApplyEvents();
    CHECK(run.client.PendingEvents() == 0);
}

TEST_CASE("[0001-CA-08] the online client sends the ping in step 3 and counts it in the summary") {
    Run run;
    run.WalkToTheWorld();
    run.client.ApplyEvents();
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
}

TEST_CASE("[0001-CA-07] a connection that closes ends the online client with the reason") {
    Run run;
    run.client.Start(Seconds{0.0});
    run.transport.Closed("connection refused");
    run.Frame(0.1);
    CHECK(run.client.Status(Seconds{0.1}) == OnlineStatus::Failed);
    CHECK(run.LogHas("the connection to the Connect Server closed: connection refused"));
}
