#pragma once

// The OpenMU 106.3 live, behind the contract of the server (ADR 0035): the transport, the session
// (Mu/Session) and the translation of the messages of the world into neutral events (Mu/Translate), as slice
// 1 made them. The account and the password come in the options and stay inside the session.

#include <cstddef>
#include <optional>
#include <string>

#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Online/Transport.hpp"
#include "Mu/Session/MessageQueue.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

class LiveServer final : public ServerAdapter {
public:
    // `transport` and `log` must outlive the server.
    LiveServer(session::SessionOptions options, Transport& transport, nexum::debug::Logger& log);

    // Opens the Connect Server.
    void Start(nexum::time::Seconds now) override;

    // Hands the events of the network to the session, enters the world when the data of the character came,
    // and translates the messages of the world.
    void TakeEvents(nexum::time::Seconds now, world::EventQueue& events) override;

    // The deadlines of the session and the ping.
    void AdvanceClock(nexum::time::Seconds now) override;

    // Nothing to do yet: the recorder of slice 2 (Mu/Recording) will take the marks of the ticks from here.
    void EventsApplied(const world::WorldView& view, std::size_t count, nexum::time::Seconds now) override;

    [[nodiscard]] session::SessionEpoch CurrentEpoch() const noexcept override { return m_session.Epoch(); }

    // Connecting until the session is in the world, and failed when the session ended after the start.
    [[nodiscard]] ServerStatus Status() const noexcept override;

    // The pings sent since the start.
    [[nodiscard]] std::string Summary(nexum::time::Seconds now) const override;

    // The state of the session, for the tests and the log; the client looks only at the status.
    [[nodiscard]] session::SessionState State() const noexcept { return m_session.State(); }

private:
    void EnterWorld();

    Transport* m_transport;
    nexum::debug::Logger* m_log;
    session::Session m_session;
    // Born with the id of the hero, which the Game Server gives (section 6.5 of the protocol spec).
    std::optional<translate::Translator> m_translator;
    bool m_started = false;
};

} // namespace mu::online
