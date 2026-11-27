#pragma once

// The session of the 106.3 client with OpenMU, from "Disconnected" to "In the world": the Connect Server, the
// list of servers, the address of the Game Server, the entry, the login, the list of characters, the choice
// and the data of the character. The order and the fields come from the protocol spec of the knowledge center
// (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.1 to 6.12 and 9); the states, from IV.14.1 of the
// reference spec.
//
// The session owns no connection. The app takes the events of each connection and hands them to Handle, with
// the epoch of that connection, and the session asks for connections and sends messages through a
// SessionLink. So the tests feed the session with messages in memory, without a network. The session never
// reads a clock either: each call brings the monotonic time of the frame.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "Mu/Protocol/ClientCodec.hpp"
#include "Mu/Protocol/Dialect.hpp"
#include "Mu/Protocol/PacketLog.hpp"
#include "Mu/Session/MessageQueue.hpp"
#include "Mu/Session/Ping.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Net/Connection.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::session {

enum class SessionState : std::uint8_t {
    Disconnected,
    ConnectServer,        // the Hello, the list of servers and the address of the Game Server
    ConnectingGameServer, // the Game Server opens and says that the player entered
    Login,                // the login was sent; its answer is awaited
    Characters,           // the list of characters was asked for
    Loading,              // the character was chosen: its data, then the map ready
    InWorld,
    // Named for the destination (IV.14.1); nothing reaches them yet.
    ChangingMap,  // M4
    Reconnecting, // M7
};

[[nodiscard]] std::string_view ToString(SessionState state) noexcept;

// Each expected answer has this long to arrive. The original has no deadline and freezes; this one is the
// Nexum's own (recommendation 5 of the protocol spec), and fixed until the configuration of M7 (TD-0027).
inline constexpr nexum::time::Seconds kAnswerDeadline{10.0};

// A connection the session asks the app to open.
struct Endpoint {
    std::string host;
    std::uint16_t port = 0;
    protocol::Encryption encryption = protocol::Encryption::None;
    SessionEpoch epoch;
};

// What the session needs from the network. The app may get these calls while it hands over an event, so it
// keeps a connection alive until the handing over ends.
class SessionLink {
public:
    virtual ~SessionLink() = default;

    // Opens a connection with a codec of its own; its events come back to Session::Handle with the epoch of
    // the endpoint. Any connection still open was closed before.
    virtual void Open(const Endpoint& endpoint) = 0;
    // Puts a plain message on the single outgoing queue of the open connection (INV-09). An error means the
    // message did not leave.
    [[nodiscard]] virtual nexum::core::Result<void> Send(std::span<const std::byte> message) = 0;
    // Closes the open connection, if any.
    virtual void Close() = 0;

protected:
    SessionLink() = default;
    SessionLink(const SessionLink&) = default;
    SessionLink& operator=(const SessionLink&) = default;
    SessionLink(SessionLink&&) = default;
    SessionLink& operator=(SessionLink&&) = default;
};

struct SessionOptions {
    std::string connectServerHost = "127.0.0.1";
    std::uint16_t connectServerPort = protocol::kConnectServerPort;
    std::string account;
    // Kept only until the login is built, then erased here; it never goes to the log (CA-12). Copies of the
    // built login stay in other buffers until they are written over (TD-0036).
    std::string password;
    // The server and the character to choose; the first of each list when there is none.
    std::optional<std::uint16_t> serverId;
    std::string character;
};

class Session {
public:
    // The link and the log must outlive the session.
    Session(SessionOptions options, SessionLink& link, nexum::debug::Logger& log);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
    ~Session();

    // Opens the Connect Server. Only from "Disconnected".
    void Start(nexum::time::Seconds now);

    // One event of the connection with this epoch. An event of an older connection is dropped.
    void Handle(SessionEpoch epoch, const nexum::net::Event& event, nexum::time::Seconds now);

    // The wall clock of the frame (step 3 of the frame, docs/ARCHITECTURE.md): it ends the session with an
    // error when the awaited answer is late, and sends the ping when it is due, from the entry in the Game
    // Server on (Mu/Session/Ping.hpp). Called once per frame, never by the ticks.
    void AdvanceClock(nexum::time::Seconds now);

    // The map of the character is ready: the session enters the world, and the messages held while loading
    // are let through. An error means the session is not loading, or the data of the character has not come.
    [[nodiscard]] nexum::core::Result<void> FinishLoading();

    // In the world, gives the messages of the world to `handler(SessionEpoch, std::span<const std::byte>)`,
    // in the order they arrived, the data of the character first. Before the world, it gives nothing and the
    // messages wait. Gives how many.
    template <typename Handler>
    std::size_t TakeWorldMessages(Handler&& handler) {
        if (m_state != SessionState::InWorld) {
            return 0;
        }
        return m_world.Drain([&](SessionEpoch epoch, std::span<const std::byte> message) {
            if (epoch == m_epoch) {
                handler(epoch, message);
            }
        });
    }

    [[nodiscard]] SessionState State() const noexcept { return m_state; }
    // True when the data of the character came and the session waits for FinishLoading.
    [[nodiscard]] bool WaitsForMap() const noexcept { return m_awaited == Awaited::MapReady; }
    [[nodiscard]] SessionEpoch Epoch() const noexcept { return m_epoch; }
    // How many pings left since the start, for the summary of the session.
    [[nodiscard]] std::uint64_t PingsSent() const noexcept { return m_pingsSent; }
    // Why the session ended; empty while it runs or when it did not fail.
    [[nodiscard]] const std::string& Failure() const noexcept { return m_failure; }
    // The id with which the player always sees itself (section 6.5); 0 before the Game Server says it.
    [[nodiscard]] std::uint16_t PlayerId() const noexcept { return m_playerId; }

private:
    // The answer the session waits for, which tells its state and what a late answer lacks.
    enum class Awaited : std::uint8_t {
        Nothing,
        Hello,
        ServerList,
        ConnectionInfo,
        GameServerEntered,
        LoginResponse,
        CharacterList,
        CharacterInformation,
        MapReady,
    };

    [[nodiscard]] static SessionState StateOf(Awaited awaited) noexcept;
    [[nodiscard]] static std::string_view Describe(Awaited awaited) noexcept;

    void HandleMessage(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnHello(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnServerList(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnConnectionInfo(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnGameServerEntered(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnLoginResponse(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnCharacterList(std::span<const std::byte> message, nexum::time::Seconds now);
    void OnLoadingMessage(std::span<const std::byte> message);
    void OnCharacterInformation(std::span<const std::byte> message);
    void HoldWorldMessage(std::span<const std::byte> message);
    void CheckDeadline(nexum::time::Seconds now);
    void SendPingWhenDue(nexum::time::Seconds now);

    void Open(std::string host, std::uint16_t port, protocol::Encryption encryption);
    // Sends the message; false means it failed, and the session already ended with the reason.
    [[nodiscard]] bool SendMessage(std::span<const std::byte> message);
    void LogPacket(protocol::PacketDirection direction, std::span<const std::byte> message);
    void SendLogin(nexum::time::Seconds now);
    void Await(Awaited awaited, nexum::time::Seconds now);
    void Fail(std::string reason);
    void Ignore(std::span<const std::byte> message);
    void EraseSecrets() noexcept;

    SessionOptions m_options;
    SessionLink* m_link;
    nexum::debug::Logger* m_log;
    SessionState m_state = SessionState::Disconnected;
    Awaited m_awaited = Awaited::Nothing;
    nexum::time::Seconds m_deadline{0.0};
    SessionEpoch m_epoch;
    std::string_view m_server = "the Connect Server"; // the server of the open connection, for the log
    std::string m_failure;
    std::uint16_t m_playerId = 0;
    MessageQueue m_world;
    bool m_worldQueueGrew = false;
    PingClock m_ping;
    std::uint64_t m_pingsSent = 0;
};

} // namespace mu::session
