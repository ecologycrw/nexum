#include "Mu/Session/Session.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "Mu/Protocol/CharacterInformation.hpp"
#include "Mu/Protocol/Dialect/ConnectServer.hpp"
#include "Mu/Protocol/Dialect/Login.hpp"
#include "Mu/Protocol/Dialect/World.hpp"
#include "Mu/Protocol/LoginMessage.hpp"

namespace mu::session {

namespace {

namespace dialect = protocol::dialect;
using nexum::core::Result;
using nexum::time::Seconds;

constexpr nexum::debug::LogCategory kSession{"session"};
constexpr nexum::debug::LogCategory kPackets{"session.packets"};

// The room of the messages held while the map loads: the same as a queue of a connection
// (Nexum/Net/Connection.hpp). The first entry of a real session has 15 to 20 messages (section 6.12).
constexpr std::size_t kHeldBytes = std::size_t{256} * 1024;
constexpr std::size_t kHeldMessages = 4096;

// A list of characters has at most 5, in the slots 0 to 4 (section 6.9).
constexpr std::size_t kMostCharacters = 5;
constexpr std::uint8_t kLastSlot = 4;

// The language byte of the request of the character list: 0 is English, and the server ignores it (6.8).
constexpr std::uint8_t kEnglish = 0;

constexpr Seconds kNoDeadline{std::numeric_limits<double>::infinity()};

// The meaning of each code of the LoginResponse (section 6.7).
[[nodiscard]] std::string_view DescribeLoginResult(dialect::LoginResponse::LoginResult result) noexcept {
    using LoginResult = dialect::LoginResponse::LoginResult;
    switch (result) {
    case LoginResult::InvalidPassword:
        return "invalid password";
    case LoginResult::Okay:
        return "accepted";
    case LoginResult::AccountInvalid:
        return "invalid account";
    case LoginResult::AccountAlreadyConnected:
        return "the account is already connected";
    case LoginResult::ServerIsFull:
        return "the server is full";
    case LoginResult::AccountBlocked:
        return "the account is blocked";
    case LoginResult::WrongVersion:
        return "wrong version";
    case LoginResult::ConnectionError:
        return "connection error";
    case LoginResult::ConnectionClosed3Fails:
        return "three failed attempts";
    case LoginResult::NoChargeInfo:
        return "no payment information";
    case LoginResult::SubscriptionTermOver:
        return "the subscription term is over";
    case LoginResult::SubscriptionTimeOver:
        return "the subscription time is over";
    case LoginResult::TemporaryBlocked:
        return "the account is blocked for a while";
    case LoginResult::OnlyPlayersOver15Yrs:
        return "only players over 15 years";
    case LoginResult::BadCountry:
        return "the country is not allowed";
    }
    return "unknown code";
}

// A packet of fixed size, with the fields `fill(writer)` writes.
template <typename Packet, typename Fill>
[[nodiscard]] Result<std::array<std::byte, Packet::kDeclaredLength>> BuildPacket(Fill&& fill) {
    std::array<std::byte, Packet::kDeclaredLength> buffer{};
    auto writer = Packet::Writer::Create(buffer);
    if (!writer) {
        return tl::unexpected(writer.error());
    }
    if (Result<void> filled = fill(*writer); !filled) {
        return tl::unexpected(filled.error());
    }
    return buffer;
}

// For a packet that has only its fixed bytes.
constexpr auto kNoFields = [](const auto& /*writer*/) -> Result<void> { return {}; };

// The server the options name, or the first of the list.
[[nodiscard]] Result<std::uint16_t> ChooseServer(const dialect::ServerListResponse::View& list,
                                                 std::optional<std::uint16_t> wanted) {
    std::string listed;
    for (std::size_t index = 0; index < list.ServersCount(); ++index) {
        auto server = list.Servers(index);
        if (!server) {
            return nexum::core::MakeError("the list of servers declares " +
                                          std::to_string(list.ServersCount()) + " servers, but holds only " +
                                          std::to_string(index));
        }
        if (!wanted || server->ServerId() == *wanted) {
            return server->ServerId();
        }
        listed += (listed.empty() ? "" : ", ") + std::to_string(server->ServerId());
    }
    if (listed.empty()) {
        return nexum::core::MakeError("the Connect Server lists no server");
    }
    return nexum::core::MakeError("the server " + std::to_string(*wanted) + " is not in the list (" + listed +
                                  ")");
}

// The character the options name, or the first of the list; the name must be the same, letter by letter, as
// the server compares it (section 6.10).
[[nodiscard]] Result<std::string> ChooseCharacter(const dialect::CharacterList::View& list,
                                                  std::string_view wanted) {
    if (list.CharactersCount() > kMostCharacters) {
        return nexum::core::MakeError("the list of characters declares " +
                                      std::to_string(list.CharactersCount()) +
                                      " characters, and an account has at most 5");
    }
    std::string listed;
    for (std::size_t index = 0; index < list.CharactersCount(); ++index) {
        auto character = list.Characters(index);
        if (!character) {
            return nexum::core::MakeError("the list of characters cannot be read: " +
                                          character.error().message);
        }
        if (character->SlotIndex() > kLastSlot) {
            return nexum::core::MakeError("the list of characters has the slot " +
                                          std::to_string(character->SlotIndex()) + "; the last is 4");
        }
        if (wanted.empty() || character->Name().Equals(wanted)) {
            return character->Name().ToString();
        }
        listed += (listed.empty() ? "" : ", ") + character->Name().ToString();
    }
    if (listed.empty()) {
        return nexum::core::MakeError("the account has no character");
    }
    return nexum::core::MakeError("the account has no character named " + std::string(wanted) + "; it has " +
                                  listed);
}

} // namespace

std::string_view ToString(SessionState state) noexcept {
    switch (state) {
    case SessionState::Disconnected:
        return "disconnected";
    case SessionState::ConnectServer:
        return "on the Connect Server";
    case SessionState::ConnectingGameServer:
        return "connecting to the Game Server";
    case SessionState::Login:
        return "logging in";
    case SessionState::Characters:
        return "choosing the character";
    case SessionState::Loading:
        return "loading";
    case SessionState::InWorld:
        return "in the world";
    case SessionState::ChangingMap:
        return "changing the map";
    case SessionState::Reconnecting:
        return "reconnecting";
    }
    return "unknown";
}

Session::Session(SessionOptions options, SessionLink& link, nexum::debug::Logger& log)
    : m_options(std::move(options)), m_link(&link), m_log(&log), m_world(kHeldBytes, kHeldMessages) {}

Session::~Session() {
    EraseSecrets();
}

SessionState Session::StateOf(Awaited awaited) noexcept {
    switch (awaited) {
    case Awaited::Nothing:
        return SessionState::InWorld;
    case Awaited::Hello:
    case Awaited::ServerList:
    case Awaited::ConnectionInfo:
        return SessionState::ConnectServer;
    case Awaited::GameServerEntered:
        return SessionState::ConnectingGameServer;
    case Awaited::LoginResponse:
        return SessionState::Login;
    case Awaited::CharacterList:
        return SessionState::Characters;
    case Awaited::CharacterInformation:
    case Awaited::MapReady:
        return SessionState::Loading;
    }
    return SessionState::Disconnected;
}

std::string_view Session::Describe(Awaited awaited) noexcept {
    switch (awaited) {
    case Awaited::Nothing:
        return "nothing";
    case Awaited::Hello:
        return "the Hello of the Connect Server";
    case Awaited::ServerList:
        return "the list of servers";
    case Awaited::ConnectionInfo:
        return "the address of the Game Server";
    case Awaited::GameServerEntered:
        return "the entry in the Game Server";
    case Awaited::LoginResponse:
        return "the answer to the login";
    case Awaited::CharacterList:
        return "the list of characters";
    case Awaited::CharacterInformation:
        return "the data of the character";
    case Awaited::MapReady:
        return "the map";
    }
    return "an answer";
}

void Session::Start(Seconds now) {
    if (m_state != SessionState::Disconnected) {
        return;
    }
    m_failure.clear();
    Open(m_options.connectServerHost, m_options.connectServerPort, protocol::Encryption::None);
    Await(Awaited::Hello, now);
}

void Session::Handle(SessionEpoch epoch, const nexum::net::Event& event, Seconds now) {
    if (m_state == SessionState::Disconnected) {
        return;
    }
    if (epoch != m_epoch) {
        // An event of a connection that is already over, such as the second Hello of the Connect Server
        // (section 6.2) or its closing.
        if (m_log->IsEnabled(nexum::debug::LogLevel::Debug)) {
            m_log->Debug(kSession, "dropped an event of the connection " + std::to_string(epoch.value));
        }
        return;
    }
    switch (event.kind) {
    case nexum::net::EventKind::Connected:
        m_log->Info(kSession, "connected to " + std::string(m_server));
        return;
    case nexum::net::EventKind::Message:
        HandleMessage(event.message, now);
        return;
    case nexum::net::EventKind::Closed:
        Fail("the connection to " + std::string(m_server) + " closed: " + std::string(event.reason));
        return;
    }
}

void Session::AdvanceClock(Seconds now) {
    CheckDeadline(now);
    SendPingWhenDue(now);
}

void Session::CheckDeadline(Seconds now) {
    if (m_state != SessionState::Disconnected && now >= m_deadline) {
        Fail(std::string(Describe(m_awaited)) + " did not come within " +
             std::to_string(static_cast<int>(kAnswerDeadline.count())) + " seconds");
    }
}

void Session::SendPingWhenDue(Seconds now) {
    auto ping = m_ping.Due(now);
    if (!ping) {
        return;
    }
    if (!*ping) {
        Fail("the ping cannot be built: " + ping->error().message);
        return;
    }
    static_cast<void>(SendMessage(**ping));
}

Result<void> Session::FinishLoading() {
    if (m_awaited != Awaited::MapReady) {
        return nexum::core::MakeError(
            "the map can only be ready after the data of the character; the session is " +
            std::string(ToString(m_state)));
    }
    Await(Awaited::Nothing, m_deadline);
    m_log->Info(kSession, "in the world");
    return {};
}

void Session::HandleMessage(std::span<const std::byte> message, Seconds now) {
    LogPacket(protocol::PacketDirection::Received, message);
    switch (m_awaited) {
    case Awaited::Hello:
        OnHello(message, now);
        return;
    case Awaited::ServerList:
        OnServerList(message, now);
        return;
    case Awaited::ConnectionInfo:
        OnConnectionInfo(message, now);
        return;
    case Awaited::GameServerEntered:
        OnGameServerEntered(message, now);
        return;
    case Awaited::LoginResponse:
        OnLoginResponse(message, now);
        return;
    case Awaited::CharacterList:
        OnCharacterList(message, now);
        return;
    case Awaited::CharacterInformation:
        OnLoadingMessage(message);
        return;
    case Awaited::MapReady:
    case Awaited::Nothing:
        HoldWorldMessage(message);
        return;
    }
}

void Session::OnHello(std::span<const std::byte> message, Seconds now) {
    if (!protocol::IsPacket(message, dialect::Hello::kShape)) {
        Ignore(message);
        return;
    }
    auto request = BuildPacket<dialect::ServerListRequest>(kNoFields);
    if (!request) {
        Fail(request.error().message);
        return;
    }
    if (SendMessage(*request)) {
        Await(Awaited::ServerList, now);
    }
}

void Session::OnServerList(std::span<const std::byte> message, Seconds now) {
    if (!protocol::IsPacket(message, dialect::ServerListResponse::kShape)) {
        Ignore(message);
        return;
    }
    auto list = dialect::ServerListResponse::View::Read(message);
    if (!list) {
        Fail("the list of servers cannot be read: " + list.error().message);
        return;
    }
    auto chosen = ChooseServer(*list, m_options.serverId);
    if (!chosen) {
        Fail(chosen.error().message);
        return;
    }
    m_log->Info(kSession, "the Connect Server lists " + std::to_string(list->ServersCount()) +
                              " servers; choosing the server " + std::to_string(*chosen));
    auto request = BuildPacket<dialect::ConnectionInfoRequest>([&](const auto& writer) -> Result<void> {
        writer.SetServerId(*chosen);
        return {};
    });
    if (!request) {
        Fail(request.error().message);
        return;
    }
    if (SendMessage(*request)) {
        Await(Awaited::ConnectionInfo, now);
    }
}

void Session::OnConnectionInfo(std::span<const std::byte> message, Seconds now) {
    if (protocol::IsPacket(message, dialect::ServerListResponse::kShape)) {
        // The Connect Server answers an unknown server with the list instead (section 6.4).
        Fail("the Connect Server answered with the list again: it does not know the chosen server");
        return;
    }
    if (!protocol::IsPacket(message, dialect::ConnectionInfo::kShape)) {
        Ignore(message);
        return;
    }
    auto info = dialect::ConnectionInfo::View::Read(message);
    if (!info) {
        Fail("the address of the Game Server cannot be read: " + info.error().message);
        return;
    }
    if (info->IpAddress().empty() || info->Port() == 0) {
        Fail("the address of the Game Server is empty");
        return;
    }
    // The client closes the Connect Server before it opens the Game Server, and always follows the address
    // it was given (section 2).
    m_link->Close();
    Open(info->IpAddress().ToString(), info->Port(), protocol::Encryption::GameServer);
    Await(Awaited::GameServerEntered, now);
}

void Session::OnGameServerEntered(std::span<const std::byte> message, Seconds now) {
    if (!protocol::IsPacket(message, dialect::GameServerEntered::kShape)) {
        Ignore(message);
        return;
    }
    auto entered = dialect::GameServerEntered::View::Read(message);
    if (!entered) {
        Fail("the entry in the Game Server cannot be read: " + entered.error().message);
        return;
    }
    if (!entered->Success()) {
        Fail("the Game Server refused the entry");
        return;
    }
    m_playerId = entered->PlayerId();
    m_log->Info(kSession, "entered the Game Server, version " + entered->VersionString().ToString());
    // Like the real client, the Nexum pings from the entry in the Game Server on (finding of the real
    // sessions, section 4).
    m_ping.Start(now);
    SendLogin(now);
}

void Session::SendLogin(Seconds now) {
    protocol::LoginMessage login{};
    auto written =
        protocol::WriteLogin(m_options.account, m_options.password, ClientMilliseconds(now), login);
    EraseSecrets();
    if (!written) {
        Fail("the login cannot be built: " + written.error().message);
        return;
    }
    const bool sent = SendMessage(login);
    // The user and the password are in the message, with a public Xor3 on them.
    std::ranges::fill(login, std::byte{0});
    if (sent) {
        Await(Awaited::LoginResponse, now);
    }
}

void Session::OnLoginResponse(std::span<const std::byte> message, Seconds now) {
    if (!protocol::IsPacket(message, dialect::LoginResponse::kShape)) {
        Ignore(message);
        return;
    }
    auto response = dialect::LoginResponse::View::Read(message);
    if (!response) {
        Fail("the answer to the login cannot be read: " + response.error().message);
        return;
    }
    const auto result = response->Success();
    if (result != dialect::LoginResponse::LoginResult::Okay) {
        // The server keeps the connection open after a refusal; this slice ends there (TD-0028).
        Fail("the login was refused: " + std::string(DescribeLoginResult(result)) + " (code " +
             std::to_string(static_cast<int>(result)) + ")");
        return;
    }
    m_log->Info(kSession, "the login was accepted");
    auto request = BuildPacket<dialect::RequestCharacterList>([](const auto& writer) -> Result<void> {
        writer.SetLanguage(kEnglish);
        return {};
    });
    if (!request) {
        Fail(request.error().message);
        return;
    }
    if (SendMessage(*request)) {
        Await(Awaited::CharacterList, now);
    }
}

void Session::OnCharacterList(std::span<const std::byte> message, Seconds now) {
    if (!protocol::IsPacket(message, dialect::CharacterList::kShape)) {
        Ignore(message);
        return;
    }
    auto list = dialect::CharacterList::View::Read(message);
    if (!list) {
        Fail("the list of characters cannot be read: " + list.error().message);
        return;
    }
    auto chosen = ChooseCharacter(*list, m_options.character);
    if (!chosen) {
        Fail(chosen.error().message);
        return;
    }
    m_log->Info(kSession, "choosing the character " + *chosen);
    auto request = BuildPacket<dialect::SelectCharacter>(
        [&](const auto& writer) -> Result<void> { return writer.SetName(*chosen); });
    if (!request) {
        Fail(request.error().message);
        return;
    }
    if (SendMessage(*request)) {
        Await(Awaited::CharacterInformation, now);
    }
}

void Session::OnLoadingMessage(std::span<const std::byte> message) {
    if (protocol::IsPacket(message, dialect::CharacterClassCreationUnlock::kShape)) {
        Ignore(message); // it may follow the list of characters (section 6.9)
        return;
    }
    if (protocol::IsPacket(message, dialect::CharacterInformation::kShape)) {
        OnCharacterInformation(message);
        return;
    }
    HoldWorldMessage(message);
}

void Session::OnCharacterInformation(std::span<const std::byte> message) {
    protocol::CharacterInformationBytes whole{};
    auto character = protocol::ReadCharacterInformation(message, whole);
    if (!character) {
        // A packet that is too short already says so; any other error comes from the reading.
        Fail(message.size() < protocol::kShortestCharacterInformation
                 ? character.error().message
                 : "the data of the character cannot be read: " + character.error().message);
        return;
    }
    m_log->Info(kSession, "the character is on the map " + std::to_string(character->MapId()) + " at (" +
                              std::to_string(character->X()) + ", " + std::to_string(character->Y()) + ")");
    m_ping.SetSpeeds(character->AttackSpeed(), character->MagicSpeed());
    HoldWorldMessage(message);
    Await(Awaited::MapReady, Seconds{0.0});
}

void Session::HoldWorldMessage(std::span<const std::byte> message) {
    // The speeds of the ping follow the last stats of the hero that arrived (section 8 and D-10, item 7).
    if (protocol::IsPacket(message, dialect::CurrentStats::kShape)) {
        if (auto stats = dialect::CurrentStats::View::Read(message)) {
            m_ping.SetSpeeds(stats->AttackSpeed(), stats->MagicSpeed());
        }
    }
    if (!m_world.Push(m_epoch, message) && !m_worldQueueGrew) {
        m_worldQueueGrew = true;
        m_log->Warning(kSession, "the messages of the world passed the reserved room; the queue grows, "
                                 "because none may be dropped");
    }
}

void Session::Open(std::string host, std::uint16_t port, protocol::Encryption encryption) {
    ++m_epoch.value;
    m_server = encryption == protocol::Encryption::None ? "the Connect Server" : "the Game Server";
    m_log->Info(kSession,
                "connecting to " + std::string(m_server) + " at " + host + ":" + std::to_string(port));
    m_link->Open({std::move(host), port, encryption, m_epoch});
}

bool Session::SendMessage(std::span<const std::byte> message) {
    LogPacket(protocol::PacketDirection::Sent, message);
    if (auto sent = m_link->Send(message); !sent) {
        Fail("could not send to " + std::string(m_server) + ": " + sent.error().message);
        return false;
    }
    return true;
}

void Session::LogPacket(protocol::PacketDirection direction, std::span<const std::byte> message) {
    if (m_log->IsEnabled(nexum::debug::LogLevel::Debug)) {
        m_log->Debug(kPackets, protocol::FormatPacket(direction, message));
    }
}

void Session::Await(Awaited awaited, Seconds now) {
    m_awaited = awaited;
    m_state = StateOf(awaited);
    const bool hasDeadline = awaited != Awaited::Nothing && awaited != Awaited::MapReady;
    m_deadline = hasDeadline ? now + kAnswerDeadline : kNoDeadline;
}

void Session::Fail(std::string reason) {
    if (m_state == SessionState::Disconnected) {
        return;
    }
    m_failure = std::move(reason);
    m_log->Error(kSession, m_failure);
    m_link->Close();
    m_state = SessionState::Disconnected;
    m_awaited = Awaited::Nothing;
    m_deadline = kNoDeadline;
    m_world.Clear();
    m_ping.Stop();
    EraseSecrets();
}

void Session::Ignore(std::span<const std::byte> message) {
    if (m_log->IsEnabled(nexum::debug::LogLevel::Debug)) {
        m_log->Debug(kSession, "while waiting for " + std::string(Describe(m_awaited)) +
                                   ", ignored a message of " + std::to_string(message.size()) + " bytes");
    }
}

void Session::EraseSecrets() noexcept {
    std::ranges::fill(m_options.password, '\0');
    m_options.password.clear();
    std::ranges::fill(m_options.account, '\0');
    m_options.account.clear();
}

} // namespace mu::session
