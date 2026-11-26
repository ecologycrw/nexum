// The transport of the real network against local test servers (specs/0001-rede-ate-o-mundo/spec.md, CA-06
// and CA-07): the session goes from the Connect Server to the Game Server over real connections, each with a
// codec of its own, and the end of a connection reaches the session with its reason. The messages come from
// SessionMessages.hpp, with their sources.
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <doctest/doctest.h>

#include "Mu/Online/NetworkTransport.hpp"
#include "Mu/Session/Session.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Net/LoopbackServer.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "SessionMessages.hpp"

namespace {

using mu::online::NetworkTransport;
using mu::session::Session;
using mu::session::SessionOptions;
using mu::session::SessionState;
using nexum::net::LoopbackServer;
using nexum::time::Seconds;
using namespace session_messages;

constexpr double kWait = 10.0;

// The time the session gets: the tests never call AdvanceClock, so no deadline runs out.
constexpr Seconds kNow{1.0};

[[nodiscard]] std::unique_ptr<LoopbackServer> StartServer() {
    auto server = LoopbackServer::Start();
    REQUIRE(server.has_value());
    return std::move(*server);
}

[[nodiscard]] SessionOptions OptionsFor(std::uint16_t connectServerPort) {
    SessionOptions options;
    options.connectServerHost = "127.0.0.1";
    options.connectServerPort = connectServerPort;
    options.account = "test0";
    options.password = "test0";
    return options;
}

// A session on the real network, with its log.
struct Run {
    explicit Run(std::uint16_t connectServerPort) : session(OptionsFor(connectServerPort), transport, log) {}

    // Hands the events to the session, as the frames would, until `count` more came, within 10 s.
    [[nodiscard]] bool DeliverEvents(std::size_t count) {
        std::size_t delivered = 0;
        for (int attempt = 0; attempt < 10'000 && delivered < count; ++attempt) {
            delivered += transport.Deliver(session, kNow);
            if (delivered < count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        return delivered >= count;
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
    NetworkTransport transport;
    Session session;
};

} // namespace

TEST_CASE("[0001-CA-07] the network transport takes the session from the Connect Server to the Game Server") {
    const auto connectServer = StartServer();
    const auto gameServer = StartServer();
    Run run(connectServer->Port());

    run.session.Start(kNow);
    REQUIRE(connectServer->Accept(kWait).has_value());
    REQUIRE(run.DeliverEvents(1));
    CHECK(run.LogHas("connected to the Connect Server"));

    // The Connect Server has no encryption: the requests leave as they are (section 2 of the protocol spec).
    REQUIRE(connectServer->Write(Hello()).has_value());
    REQUIRE(run.DeliverEvents(1));
    const auto listRequest = connectServer->Read(4, kWait);
    REQUIRE(listRequest.has_value());
    CHECK(*listRequest == Make({0xC1, 0x04, 0xF4, 0x06}));

    REQUIRE(connectServer->Write(ServerList()).has_value());
    REQUIRE(run.DeliverEvents(1));
    const auto addressRequest = connectServer->Read(6, kWait);
    REQUIRE(addressRequest.has_value());
    CHECK(*addressRequest == Make({0xC1, 0x06, 0xF4, 0x03, 0x00, 0x00}));

    // The session closes the Connect Server while it handles the address, and opens the Game Server.
    REQUIRE(connectServer->Write(ConnectionInfo(gameServer->Port())).has_value());
    REQUIRE(run.DeliverEvents(1));
    CHECK(connectServer->WaitForClientToClose(kWait));
    REQUIRE(gameServer->Accept(kWait).has_value());
    REQUIRE(run.DeliverEvents(1));
    CHECK(run.LogHas("connected to the Game Server"));

    // The login leaves encrypted, as a C3 (sections 4 and 6.6).
    REQUIRE(gameServer->Write(GameServerEntered()).has_value());
    REQUIRE(run.DeliverEvents(1));
    CHECK(run.session.State() == SessionState::Login);
    const auto header = gameServer->Read(2, kWait);
    REQUIRE(header.has_value());
    CHECK((*header)[0] == std::byte{0xC3});
    const auto length = std::to_integer<std::size_t>((*header)[1]);
    REQUIRE(length > 2);
    CHECK(gameServer->Read(length - 2, kWait).has_value());

    // The Game Server goes away: the session ends with the reason.
    gameServer->CloseClient();
    REQUIRE(run.DeliverEvents(1));
    CHECK(run.session.State() == SessionState::Disconnected);
    CHECK(run.session.Failure().starts_with("the connection to the Game Server closed: "));
}

TEST_CASE("[0001-CA-07] a Connect Server that refuses the connection ends the session with the reason") {
    const auto connectServer = StartServer();
    connectServer->StopListening();
    Run run(connectServer->Port());

    run.session.Start(kNow);
    REQUIRE(run.DeliverEvents(1));
    CHECK(run.session.State() == SessionState::Disconnected);
    CHECK(run.session.Failure().starts_with("the connection to the Connect Server closed: "));
}
