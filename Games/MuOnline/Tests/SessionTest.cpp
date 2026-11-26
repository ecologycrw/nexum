// The session from "Disconnected" to "In the world" (CA-07), fed with messages in memory and without a
// network, and its log without the user and the password (CA-12). The messages and their order come from the
// protocol spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.1 to 6.12 and
// 9) and from the examples of the plan of slice 1; the bytes of a real session come from the finding
// 2026-11-21-capturas-reais-sessao-106-3.md (section 3).
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/Cipher.hpp"
#include "Mu/Protocol/Dialect/World.hpp"
#include "Mu/Protocol/LoginMessage.hpp"
#include "Mu/Session/Session.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

namespace {

using mu::session::Endpoint;
using mu::session::Session;
using mu::session::SessionEpoch;
using mu::session::SessionOptions;
using mu::session::SessionState;
using nexum::time::Seconds;
using Bytes = std::vector<std::byte>;

[[nodiscard]] Bytes Make(std::initializer_list<std::uint8_t> values) {
    Bytes bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

[[nodiscard]] Bytes Text(std::string_view text, std::size_t size) {
    Bytes bytes(size);
    for (std::size_t index = 0; index < text.size(); ++index) {
        bytes[index] = static_cast<std::byte>(text[index]);
    }
    return bytes;
}

// The first 4 bytes, as the log of packets writes them: "70 6C 61 79".
[[nodiscard]] std::string Hex(const Bytes& bytes) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::string text;
    for (std::size_t index = 0; index < 4; ++index) {
        const auto value = std::to_integer<std::uint8_t>(bytes[index]);
        text += index == 0 ? "" : " ";
        text += kDigits[value >> 4U];
        text += kDigits[value & 0x0FU];
    }
    return text;
}

[[nodiscard]] Bytes Join(std::initializer_list<Bytes> parts) {
    Bytes joined;
    for (const auto& part : parts) {
        joined.insert(joined.end(), part.begin(), part.end());
    }
    return joined;
}

// The messages of the session, as the server sends them (sections 6.2 to 6.11).
[[nodiscard]] Bytes Hello() {
    return Make({0xC1, 0x04, 0x00, 0x01});
}

// The list of a real session: 3 servers, with the ids 0, 1 and 2.
[[nodiscard]] Bytes ServerList() {
    return Make({0xC2, 0x00, 0x13, 0xF4, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                 0x02, 0x00, 0x00, 0x00});
}

// 127.0.0.1, port 45902 (0xB34E, little-endian).
[[nodiscard]] Bytes ConnectionInfo() {
    return Join({Make({0xC1, 0x16, 0xF4, 0x03}), Text("127.0.0.1", 16), Make({0x4E, 0xB3})});
}

// The entry of a real session: success, the player id 0x0200 and the version "20404".
[[nodiscard]] Bytes GameServerEntered() {
    return Make({0xC1, 0x0C, 0xF1, 0x00, 0x01, 0x02, 0x00, 0x32, 0x30, 0x34, 0x30, 0x34});
}

[[nodiscard]] Bytes LoginResponse(std::uint8_t result) {
    return Make({0xC1, 0x05, 0xF1, 0x01, result});
}

// A list with these characters in the slots 0, 1...: 8 bytes, then 44 for each (section 6.9).
[[nodiscard]] Bytes CharacterList(std::initializer_list<std::string_view> names) {
    const auto count = static_cast<std::uint8_t>(names.size());
    Bytes list =
        Make({0xC1, static_cast<std::uint8_t>(8 + (44 * count)), 0xF3, 0x00, 0x00, 0x00, count, 0x00});
    std::uint8_t slot = 0;
    for (const std::string_view name : names) {
        Bytes character(44);
        character[0] = std::byte{slot++};
        const Bytes text = Text(name, 10);
        std::copy(text.begin(), text.end(), character.begin() + 1);
        character[42] = std::byte{0xFF}; // no guild
        list.insert(list.end(), character.begin(), character.end());
    }
    return list;
}

// The data of the character, of 93 bytes or fewer, at (145, 123) on the map 0, with the attack and magic
// speeds 29 and 29 (bytes 82 to 85, little-endian), as in a real session.
[[nodiscard]] Bytes CharacterInformation(std::size_t size = 93) {
    Bytes data(size);
    data[0] = std::byte{0xC3};
    data[1] = std::byte{static_cast<std::uint8_t>(size)};
    data[2] = std::byte{0xF3};
    data[3] = std::byte{0x03};
    data[4] = std::byte{145};
    data[5] = std::byte{123};
    if (size >= 86) {
        data[82] = std::byte{29};
        data[84] = std::byte{29};
    }
    return data;
}

// The stats of the hero (C1 26 FF), with new attack and magic speeds.
[[nodiscard]] Bytes CurrentStats(std::uint16_t attackSpeed, std::uint16_t magicSpeed) {
    Bytes stats(mu::protocol::dialect::CurrentStats::kShape.minimumLength);
    auto writer = mu::protocol::dialect::CurrentStats::Writer::Create(stats);
    REQUIRE(writer.has_value());
    writer->SetAttackSpeed(attackSpeed);
    writer->SetMagicSpeed(magicSpeed);
    return stats;
}

// The pings among the messages sent, each as its time in ms and its two speeds.
struct SentPing {
    std::uint32_t milliseconds;
    std::uint16_t attackSpeed;
    std::uint16_t magicSpeed;
};

[[nodiscard]] std::vector<SentPing> Pings(const std::vector<Bytes>& sent) {
    std::vector<SentPing> pings;
    for (const Bytes& message : sent) {
        if (auto ping = mu::protocol::dialect::Ping::View::Read(message)) {
            pings.push_back({ping->TickCount(), ping->AttackSpeed(), ping->MagicSpeed()});
        }
    }
    return pings;
}

// Messages of the world of the first entry (section 6.12 and the plan of slice 1).
[[nodiscard]] Bytes HeroEntered() {
    Bytes hero = Make({0xC2, 0x00, 0x36, 0x12, 0x00, 0x02, 0x91, 0x7B, 0x91, 0x7B, 0x00, 0x00,
                       0x1D, 0x00, 0x1D, 0x00, 0x74, 0x65, 0x73, 0x74, 0x30, 0x44, 0x6B});
    hero.resize(54);
    return hero;
}

[[nodiscard]] Bytes NpcEntered() {
    return Make({0xC2, 0x00, 0x0F, 0x13, 0x01, 0x80, 0x21, 0x00, 0x0E, 0x87, 0x7C, 0x87, 0x7C, 0x30, 0x00});
}

[[nodiscard]] Bytes ObjectLeft() {
    return Make({0xC1, 0x06, 0x14, 0x01, 0x00, 0x21});
}

// What the session asked of the network, kept in memory.
class RecordingLink final : public mu::session::SessionLink {
public:
    void Open(const Endpoint& endpoint) override { opened.push_back(endpoint); }

    nexum::core::Result<void> Send(std::span<const std::byte> message) override {
        if (refuseSends) {
            return nexum::core::MakeError("the outgoing queue is full");
        }
        sent.emplace_back(message.begin(), message.end());
        return {};
    }

    void Close() override { ++closes; }

    std::vector<Endpoint> opened;
    std::vector<Bytes> sent;
    int closes = 0;
    bool refuseSends = false;
};

[[nodiscard]] SessionOptions DefaultOptions() {
    SessionOptions options;
    options.account = "test0";
    options.password = "test0";
    return options;
}

// A session with its link and its log, and the steps of the way to the world.
struct Run {
    explicit Run(SessionOptions options = DefaultOptions()) : session(std::move(options), link, log) {}

    void Message(SessionEpoch epoch, const Bytes& message, double now) {
        session.Handle(epoch, {nexum::net::EventKind::Message, message, {}}, Seconds{now});
    }

    void Message(const Bytes& message, double now) { Message(session.Epoch(), message, now); }

    void Connected(double now) {
        session.Handle(session.Epoch(), {nexum::net::EventKind::Connected, {}, {}}, Seconds{now});
    }

    void Closed(SessionEpoch epoch, std::string_view reason, double now) {
        session.Handle(epoch, {nexum::net::EventKind::Closed, {}, reason}, Seconds{now});
    }

    // The way to the world, one answer of the server for each step, 0.1 s apart: after `steps` of them, the
    // session waits for the next one.
    void Walk(int steps) {
        const std::vector<Bytes> answers{Hello(),
                                         ServerList(),
                                         ConnectionInfo(),
                                         GameServerEntered(),
                                         LoginResponse(1),
                                         CharacterList({"test0Dk"}),
                                         CharacterInformation()};
        session.Start(Seconds{0.0});
        for (int step = 0; step < steps; ++step) {
            const double now = 0.1 * (step + 1);
            if (step == 3) {
                Connected(now);
            }
            Message(answers[static_cast<std::size_t>(step)], now);
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
    nexum::debug::Logger log{sink, nexum::debug::LogLevel::Debug};
    RecordingLink link;
    Session session;
};

[[nodiscard]] std::vector<Bytes> TakeWorld(Session& session) {
    std::vector<Bytes> taken;
    session.TakeWorldMessages([&](SessionEpoch epoch, std::span<const std::byte> message) {
        CHECK(epoch == session.Epoch());
        taken.emplace_back(message.begin(), message.end());
    });
    return taken;
}

} // namespace

TEST_CASE(
    "[0001-CA-07] the session goes from Disconnected to In the world in the order of the protocol spec") {
    Run run;
    CHECK(run.session.State() == SessionState::Disconnected);

    run.session.Start(Seconds{0.0});
    CHECK(run.session.State() == SessionState::ConnectServer);
    REQUIRE(run.link.opened.size() == 1);
    CHECK(run.link.opened[0].host == "127.0.0.1");
    CHECK(run.link.opened[0].port == 44406);
    CHECK(run.link.opened[0].encryption == mu::protocol::Encryption::None);
    CHECK(run.link.opened[0].epoch == SessionEpoch{1});

    run.Connected(0.05);
    run.Message(Hello(), 0.1);
    REQUIRE(run.link.sent.size() == 1);
    CHECK(run.link.sent[0] == Make({0xC1, 0x04, 0xF4, 0x06}));

    // The Nexum chooses the first server, the 0.
    run.Message(ServerList(), 0.2);
    REQUIRE(run.link.sent.size() == 2);
    CHECK(run.link.sent[1] == Make({0xC1, 0x06, 0xF4, 0x03, 0x00, 0x00}));

    // The Connect Server closes before the Game Server opens, at the address it gave.
    run.Message(ConnectionInfo(), 0.3);
    CHECK(run.session.State() == SessionState::ConnectingGameServer);
    CHECK(run.link.closes == 1);
    REQUIRE(run.link.opened.size() == 2);
    CHECK(run.link.opened[1].host == "127.0.0.1");
    CHECK(run.link.opened[1].port == 45902);
    CHECK(run.link.opened[1].encryption == mu::protocol::Encryption::GameServer);
    CHECK(run.link.opened[1].epoch == SessionEpoch{2});

    // The second Hello of the Connect Server and its closing belong to the old connection.
    run.Message(SessionEpoch{1}, Hello(), 0.31);
    run.Closed(SessionEpoch{1}, "closed by the client", 0.32);
    CHECK(run.session.State() == SessionState::ConnectingGameServer);
    CHECK(run.link.sent.size() == 2);

    // The login, with the user and the password "test0", the time in ms, the version and the serial.
    run.Connected(0.4);
    run.Message(GameServerEntered(), 0.5);
    CHECK(run.session.State() == SessionState::Login);
    CHECK(run.session.PlayerId() == 0x0200);
    mu::protocol::LoginMessage login{};
    REQUIRE(mu::protocol::WriteLogin("test0", "test0", 500, login).has_value());
    REQUIRE(run.link.sent.size() == 3);
    CHECK(run.link.sent[2] == Bytes(login.begin(), login.end()));
    CHECK(Bytes(run.link.sent[2].begin() + 4, run.link.sent[2].begin() + 14) ==
          Make({0x88, 0xAA, 0xD8, 0x88, 0xFF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC}));

    run.Message(LoginResponse(1), 0.6);
    CHECK(run.session.State() == SessionState::Characters);
    REQUIRE(run.link.sent.size() == 4);
    CHECK(run.link.sent[3] == Make({0xC1, 0x05, 0xF3, 0x00, 0x00}));

    run.Message(CharacterList({"test0Dk"}), 0.7);
    CHECK(run.session.State() == SessionState::Loading);
    REQUIRE(run.link.sent.size() == 5);
    CHECK(run.link.sent[4] == Join({Make({0xC1, 0x0E, 0xF3, 0x03}), Text("test0Dk", 10)}));

    run.Message(CharacterInformation(), 0.8);
    CHECK(run.session.State() == SessionState::Loading);
    REQUIRE(run.session.FinishLoading().has_value());
    CHECK(run.session.State() == SessionState::InWorld);
    CHECK(run.link.sent.size() == 5);
    CHECK(run.session.Failure().empty());
}

TEST_CASE("[0001-CA-07] what arrives while loading is held and let through in order when the hero enters") {
    Run run;
    run.Walk(6);
    // The unlock of classes may follow the list of characters, and belongs to the session (section 6.9).
    run.Message(Make({0xC1, 0x05, 0xDE, 0x00, 0x01}), 0.65);
    run.Message(CharacterInformation(), 0.8);
    run.Message(HeroEntered(), 0.81);
    run.Message(NpcEntered(), 0.82);
    run.Message(ObjectLeft(), 0.83);

    CHECK(TakeWorld(run.session).empty());

    REQUIRE(run.session.FinishLoading().has_value());
    const std::vector<Bytes> taken = TakeWorld(run.session);
    REQUIRE(taken.size() == 4);
    CHECK(taken[0] == CharacterInformation());
    CHECK(taken[1] == HeroEntered());
    CHECK(taken[2] == NpcEntered());
    CHECK(taken[3] == ObjectLeft());

    // In the world, each message waits only for the next tick.
    run.Message(ObjectLeft(), 0.9);
    const std::vector<Bytes> next = TakeWorld(run.session);
    REQUIRE(next.size() == 1);
    CHECK(next[0] == ObjectLeft());
    CHECK(TakeWorld(run.session).empty());
}

TEST_CASE("[0001-CA-07] the map is ready only after the data of the character") {
    Run run;
    run.Walk(6);
    CHECK_FALSE(run.session.FinishLoading().has_value());
    CHECK(run.session.State() == SessionState::Loading);
}

TEST_CASE("[0001-CA-07] a refused login shows the meaning of the code and ends with an error") {
    struct Refusal {
        std::uint8_t code;
        std::string_view failure;
    };
    for (const Refusal refusal :
         {Refusal{0, "the login was refused: invalid password (code 0)"},
          Refusal{3, "the login was refused: the account is already connected (code 3)"},
          Refusal{14, "the login was refused: the account is blocked for a while (code 14)"},
          Refusal{99, "the login was refused: unknown code (code 99)"}}) {
        CAPTURE(refusal.code);
        Run run;
        run.Walk(4);
        run.Message(LoginResponse(refusal.code), 0.6);
        CHECK(run.session.State() == SessionState::Disconnected);
        CHECK(run.session.Failure() == refusal.failure);
        CHECK(run.link.closes == 2);
        CHECK(run.LogHas(refusal.failure));
    }
}

TEST_CASE("[0001-CA-07] each expected answer has 10 seconds, and a late one says what was missing") {
    const std::vector<std::string_view> awaited{
        "the Hello of the Connect Server", "the list of servers",     "the address of the Game Server",
        "the entry in the Game Server",    "the answer to the login", "the list of characters",
        "the data of the character"};
    for (std::size_t steps = 0; steps < awaited.size(); ++steps) {
        CAPTURE(steps);
        Run run;
        run.Walk(static_cast<int>(steps));
        const double sent = 0.1 * static_cast<double>(steps);
        run.session.AdvanceClock(Seconds{sent + 9.99});
        CHECK(run.session.State() != SessionState::Disconnected);
        run.session.AdvanceClock(Seconds{sent + 10.0});
        CHECK(run.session.State() == SessionState::Disconnected);
        CHECK(run.session.Failure() == std::string(awaited[steps]) + " did not come within 10 seconds");
    }
}

TEST_CASE("[0001-CA-07] the loading of the map and the world have no deadline") {
    Run run;
    run.Walk(7);
    run.session.AdvanceClock(Seconds{3600.0});
    CHECK(run.session.State() == SessionState::Loading);
    REQUIRE(run.session.FinishLoading().has_value());
    run.session.AdvanceClock(Seconds{7200.0});
    CHECK(run.session.State() == SessionState::InWorld);
}

TEST_CASE("[0001-CA-07] the data of the character with fewer than 92 bytes is an error, and 92 are enough") {
    Run shortRun;
    shortRun.Walk(6);
    shortRun.Message(CharacterInformation(91), 0.8);
    CHECK(shortRun.session.State() == SessionState::Disconnected);
    CHECK(shortRun.session.Failure() == "the data of the character has 91 bytes; it needs at least 92");

    Run enough;
    enough.Walk(6);
    enough.Message(CharacterInformation(92), 0.8);
    REQUIRE(enough.session.FinishLoading().has_value());
    const std::vector<Bytes> taken = TakeWorld(enough.session);
    REQUIRE(taken.size() == 1);
    CHECK(taken[0] == CharacterInformation(92));
    CHECK(enough.LogHas("the character is on the map 0 at (145, 123)"));
}

TEST_CASE("[0001-CA-07] the epoch grows with each connection, and an event of an old epoch is dropped") {
    Run run;
    CHECK(run.session.Epoch() == SessionEpoch{0});
    run.session.Start(Seconds{0.0});
    CHECK(run.session.Epoch() == SessionEpoch{1});
    run.Message(Hello(), 0.1);
    run.Message(ServerList(), 0.2);
    run.Message(ConnectionInfo(), 0.3);
    CHECK(run.session.Epoch() == SessionEpoch{2});

    // Nothing of the Connect Server counts any more: neither a message nor its closing.
    run.Message(SessionEpoch{1}, GameServerEntered(), 0.4);
    CHECK(run.session.State() == SessionState::ConnectingGameServer);
    run.Closed(SessionEpoch{1}, "closed by the server", 0.4);
    CHECK(run.session.State() == SessionState::ConnectingGameServer);
    CHECK(run.session.Failure().empty());

    run.Message(GameServerEntered(), 0.5);
    CHECK(run.session.State() == SessionState::Login);
}

TEST_CASE("[0001-CA-07] a closed connection ends the session with the reason, without reconnecting") {
    Run run;
    run.Walk(7);
    REQUIRE(run.session.FinishLoading().has_value());
    run.Closed(run.session.Epoch(), "the server closed the connection", 5.0);
    CHECK(run.session.State() == SessionState::Disconnected);
    CHECK(run.session.Failure() ==
          "the connection to the Game Server closed: the server closed the connection");
    CHECK(run.link.opened.size() == 2);
    CHECK(TakeWorld(run.session).empty());
}

TEST_CASE("[0001-CA-07] the session chooses the server and the character that the options name") {
    SessionOptions options = DefaultOptions();
    options.serverId = 2;
    options.character = "second";
    Run run(options);
    run.Walk(2);
    REQUIRE(run.link.sent.size() == 2);
    CHECK(run.link.sent[1] == Make({0xC1, 0x06, 0xF4, 0x03, 0x02, 0x00}));
    run.Message(ConnectionInfo(), 0.3);
    run.Connected(0.4);
    run.Message(GameServerEntered(), 0.4);
    run.Message(LoginResponse(1), 0.5);
    run.Message(CharacterList({"first", "second"}), 0.6);
    REQUIRE(run.link.sent.size() == 5);
    CHECK(run.link.sent[4] == Join({Make({0xC1, 0x0E, 0xF3, 0x03}), Text("second", 10)}));
}

TEST_CASE("[0001-CA-07] a server or a character that does not exist ends the session with what exists") {
    SessionOptions noServer = DefaultOptions();
    noServer.serverId = 7;
    Run serverRun(noServer);
    serverRun.Walk(2);
    CHECK(serverRun.session.Failure() == "the server 7 is not in the list (0, 1, 2)");

    SessionOptions noCharacter = DefaultOptions();
    noCharacter.character = "nobody";
    Run characterRun(noCharacter);
    characterRun.Walk(5);
    characterRun.Message(CharacterList({"first", "second"}), 0.6);
    CHECK(characterRun.session.Failure() ==
          "the account has no character named nobody; it has first, second");

    Run emptyRun;
    emptyRun.Walk(5);
    emptyRun.Message(CharacterList({}), 0.6);
    CHECK(emptyRun.session.Failure() == "the account has no character");
}

TEST_CASE("[0001-CA-07] the Connect Server answering with the list again ends the session") {
    Run run;
    run.Walk(2);
    run.Message(ServerList(), 0.3);
    CHECK(run.session.State() == SessionState::Disconnected);
    CHECK(run.session.Failure() == "the Connect Server answered with the list again: it does not know the "
                                   "chosen server");
}

TEST_CASE("[0001-CA-07] a message that cannot be sent ends the session") {
    Run run;
    run.link.refuseSends = true;
    run.Walk(1);
    CHECK(run.session.State() == SessionState::Disconnected);
    CHECK(run.session.Failure() == "could not send to the Connect Server: the outgoing queue is full");
}

TEST_CASE("[0001-CA-07] a message the session does not wait for is ignored") {
    Run run;
    run.Walk(1);
    run.Message(Hello(), 0.15);
    run.Message(LoginResponse(1), 0.16);
    CHECK(run.session.State() == SessionState::ConnectServer);
    CHECK(run.link.sent.size() == 1);
    run.Message(ServerList(), 0.2);
    CHECK(run.link.sent.size() == 2);
}

TEST_CASE("[0001-CA-12] the log of the session never shows the user or the password") {
    SessionOptions options = DefaultOptions();
    options.account = "player1";
    options.password = "Pa55word";
    Run run(options);
    run.Walk(7);
    REQUIRE(run.session.FinishLoading().has_value());

    // The login went out with both, and the log shows its bytes with the credentials hidden.
    REQUIRE(run.link.sent.size() >= 3);
    std::string hidden = "sent 60 bytes: C3 3C F1 01";
    for (int index = 0; index < 30; ++index) {
        hidden += " **";
    }
    CHECK(run.LogHas(hidden));

    // Neither the text nor its first bytes, plain or with the public Xor3 (section 4.4), reach any line of
    // the log.
    Bytes user = Text("player1", 10);
    Bytes password = Text("Pa55word", 20);
    std::vector<std::string> secrets{"player1", "Pa55word", Hex(user), Hex(password)};
    mu::protocol::Xor3(user);
    mu::protocol::Xor3(password);
    secrets.push_back(Hex(user));
    secrets.push_back(Hex(password));
    for (const auto& entry : run.sink.Entries()) {
        CAPTURE(entry.message);
        for (const auto& secret : secrets) {
            CHECK(entry.message.find(secret) == std::string::npos);
        }
    }
}

namespace {

// The way to the world with the entry in the Game Server at t = 0, as in the example of the plan.
void EnterAtZero(Run& run) {
    run.session.Start(Seconds{0.0});
    run.Message(Hello(), 0.0);
    run.Message(ServerList(), 0.0);
    run.Message(ConnectionInfo(), 0.0);
    run.Connected(0.0);
    run.Message(GameServerEntered(), 0.0);
}

} // namespace

TEST_CASE("[0001-CA-08] the ping goes every 20 s of the wall clock from the entry in the Game Server, "
          "whatever the frames") {
    Run run;
    EnterAtZero(run);
    run.Message(LoginResponse(1), 0.1);
    run.Message(CharacterList({"test0Dk"}), 0.2);
    run.Message(CharacterInformation(), 0.3);
    REQUIRE(run.session.FinishLoading().has_value());

    struct Frame {
        double now;
        std::size_t pings;
    };
    for (const Frame frame :
         {Frame{0.016, 0}, Frame{5.0, 0}, Frame{19.999, 0}, Frame{20.0, 1}, Frame{20.016, 1}, Frame{39.5, 1},
          Frame{40.7, 2}, Frame{41.0, 2}, Frame{59.999, 2}, Frame{60.0, 3}}) {
        CAPTURE(frame.now);
        run.session.AdvanceClock(Seconds{frame.now});
        CHECK(Pings(run.link.sent).size() == frame.pings);
    }
    const std::vector<SentPing> pings = Pings(run.link.sent);
    REQUIRE(pings.size() == 3);
    CHECK(pings[0].milliseconds == 20000);
    CHECK(pings[1].milliseconds == 40700);
    CHECK(pings[2].milliseconds == 60000);
}

TEST_CASE("[0001-CA-08] a long stall gives one ping, and the next stays on the rhythm of 20 s") {
    Run run;
    EnterAtZero(run);
    run.Message(LoginResponse(1), 0.1);
    run.Message(CharacterList({"test0Dk"}), 0.2);
    run.Message(CharacterInformation(), 0.3);
    REQUIRE(run.session.FinishLoading().has_value());

    run.session.AdvanceClock(Seconds{65.0});
    CHECK(Pings(run.link.sent).size() == 1);
    run.session.AdvanceClock(Seconds{79.9});
    CHECK(Pings(run.link.sent).size() == 1);
    run.session.AdvanceClock(Seconds{80.0});
    CHECK(Pings(run.link.sent).size() == 2);
}

TEST_CASE("[0001-CA-08] the ping carries the speeds last known, 0 and 0 before the character enters") {
    Run run;
    EnterAtZero(run);
    run.Message(LoginResponse(1), 9.0);
    run.Message(CharacterList({"test0Dk"}), 18.0);

    // Still waiting for the data of the character: the ping goes, with the speeds 0 and 0, on the single
    // outgoing queue, after the choice of the character.
    run.session.AdvanceClock(Seconds{20.0});
    REQUIRE(run.link.sent.size() == 6);
    CHECK(run.link.sent[5] == Make({0xC3, 0x0C, 0x0E, 0x00, 0x20, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));

    run.Message(CharacterInformation(), 21.0);
    REQUIRE(run.session.FinishLoading().has_value());
    run.session.AdvanceClock(Seconds{40.0});
    run.Message(CurrentStats(35, 40), 41.0);
    run.session.AdvanceClock(Seconds{60.0});

    const std::vector<SentPing> pings = Pings(run.link.sent);
    REQUIRE(pings.size() == 3);
    CHECK(pings[0].attackSpeed == 0);
    CHECK(pings[0].magicSpeed == 0);
    CHECK(pings[1].attackSpeed == 29);
    CHECK(pings[1].magicSpeed == 29);
    CHECK(pings[2].attackSpeed == 35);
    CHECK(pings[2].magicSpeed == 40);
}

TEST_CASE("[0001-CA-08] no ping goes before the entry in the Game Server, nor after the session ends") {
    Run run;
    run.session.Start(Seconds{0.0});
    run.Message(Hello(), 9.0);
    run.Message(ServerList(), 18.0);
    run.session.AdvanceClock(Seconds{25.0});
    run.Message(ConnectionInfo(), 27.0);
    run.Connected(30.0);
    run.Message(GameServerEntered(), 30.0);
    run.Message(LoginResponse(1), 39.0);
    run.Message(CharacterList({"test0Dk"}), 45.0);
    run.Message(CharacterInformation(), 47.0);
    REQUIRE(run.session.FinishLoading().has_value());

    run.session.AdvanceClock(Seconds{49.9});
    CHECK(Pings(run.link.sent).empty());
    run.session.AdvanceClock(Seconds{50.0});
    CHECK(Pings(run.link.sent).size() == 1);

    run.Closed(run.session.Epoch(), "the server closed the connection", 55.0);
    run.session.AdvanceClock(Seconds{70.0});
    run.session.AdvanceClock(Seconds{90.0});
    CHECK(Pings(run.link.sent).size() == 1);
}
