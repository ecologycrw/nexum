#pragma once

// The single contract between the online client and the server it plays with (ADR 0035): the OpenMU 106.3
// live (LiveServer) or, from slice 2 on, the replay of a recording. The client takes the events of the world
// from it, runs its wall clocks and tells it after each tick how the view of the world ended up, without
// knowing where the events come from. The contract knows nothing of bytes, connections or files, and never
// sees the account or the password: they go only to the live implementation, when it is born.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

enum class ServerState : std::uint8_t {
    // From the start until the hero is in the world.
    Connecting,
    InWorld,
    // The server ended without a failure, as a replay that reached its end.
    Ended,
    // The server ended with an error; the status says why.
    Failed,
};

struct ServerStatus {
    ServerState state = ServerState::Connecting;
    // Why the server failed; empty in the other states. Valid until the next call to the server.
    std::string_view reason;
};

class ServerAdapter {
public:
    virtual ~ServerAdapter() = default;

    // The start of the session, once, before the first frame.
    virtual void Start(nexum::time::Seconds now) = 0;

    // Step 2 of the frame: puts at the end of `events` the events of the world that came since the last call,
    // in their order, each with the epoch of its message. It never waits for the network or the disk
    // (INV-06).
    virtual void TakeEvents(nexum::time::Seconds now, world::EventQueue& events) = 0;

    // Step 3 of the frame: the wall clocks of the server, such as the deadlines and the ping.
    virtual void AdvanceClock(nexum::time::Seconds now) = 0;

    // After each tick that took events out of the queue: how many it took, applied or dropped for their
    // epoch, and the view as they left it. `now` is the time of the frame of the tick.
    virtual void EventsApplied(const world::WorldView& view, std::size_t count, nexum::time::Seconds now) = 0;

    // The epoch of the events the server gives now; the tick drops each event of an older epoch, which is
    // from a session that is over.
    [[nodiscard]] virtual session::SessionEpoch CurrentEpoch() const noexcept = 0;

    [[nodiscard]] virtual ServerStatus Status() const noexcept = 0;

    // A few words about the server for the last line of the client, such as "pings sent: 9".
    [[nodiscard]] virtual std::string Summary(nexum::time::Seconds now) const = 0;

protected:
    ServerAdapter() = default;
    ServerAdapter(const ServerAdapter&) = default;
    ServerAdapter& operator=(const ServerAdapter&) = default;
    ServerAdapter(ServerAdapter&&) = default;
    ServerAdapter& operator=(ServerAdapter&&) = default;
};

} // namespace mu::online
