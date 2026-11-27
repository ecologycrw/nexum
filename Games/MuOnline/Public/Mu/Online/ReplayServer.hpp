#pragma once

// The replay of a .mucap recording (Mu/Recording/Recording.hpp), the second implementation of the contract of
// the server (ADR 0035, and the plan of slice 2, item 9): the client plays the recorded session without a
// server, and the replay checks, at each tick of the recording, that the client got to the same place.
//
// The records come in groups, each ending with the entry in the world, a tick or the end. A group is handed
// over in step 2 of the first frame whose time, counted from the Start, reached the time of its last record:
// the messages go to the translation with their epoch, and the changes of epoch move the current epoch. After
// a group that ends with a tick, the replay waits: the next tick of the client takes exactly the events of
// the group, as the recorded tick did, and then the replay compares how many it took, applied or dropped,
// with the mark, and the view with the checkpoint that follows the mark, when there is one. Only then does
// the next group come. So the grouping by tick is the one of the recorded session, whatever the pace of the
// frames, and a change of epoch drops the same events.
//
// A difference ends the replay with an error that names the tick or the checkpoint, with the recorded and
// the replayed values. The end of the recording ends the replay without a failure, however the recorded
// session ended; the events of the messages after the last tick, which no recorded tick applied, are handed
// over with the end and checked by nothing.
//
// A tick of the client that takes nothing never reaches the server, so the recorder never writes a tick
// without events. A group whose messages give no events is therefore checked at once against 0 events: a
// mark with more is a difference; a mark with 0, which only a recording made by hand has, passes, but a
// checkpoint after it is a difference, because no tick brings the view to compare.
//
// Handing over the records and checking them never asks for memory (INV-13); only a difference builds its
// message, once.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Recording/Recording.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/LogLine.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

class ReplayServer final : public ServerAdapter {
public:
    // `recording`, already read and checked, and `log` must outlive the server.
    ReplayServer(const recording::Recording& recording, nexum::debug::Logger& log);

    // The time 0 of the recording.
    void Start(nexum::time::Seconds now) override;

    // Hands over the next group of records whose time came, unless the replay waits for a tick.
    void TakeEvents(nexum::time::Seconds now, world::EventQueue& events) override;

    // The replay has no wall clocks.
    void AdvanceClock(nexum::time::Seconds /*now*/) override {}

    // Compares the tick the replay waits for, and the checkpoint after it. Any other tick, as one after the
    // end, is not in the recording, and is not checked.
    void EventsApplied(const world::WorldView& view, std::size_t count, nexum::time::Seconds now) override;

    [[nodiscard]] session::SessionEpoch CurrentEpoch() const noexcept override { return m_epoch; }

    // Connecting until the entry in the world of the recording, ended at its end, and failed at the first
    // difference, with its message.
    [[nodiscard]] ServerStatus Status() const noexcept override;

    // How many messages, ticks and checkpoints the replay checked.
    [[nodiscard]] std::string Summary(nexum::time::Seconds now) const override;

    [[nodiscard]] std::size_t Messages() const noexcept { return m_messages; }
    [[nodiscard]] std::size_t Ticks() const noexcept { return m_ticks; }
    [[nodiscard]] std::size_t Checkpoints() const noexcept { return m_checkpoints; }

private:
    enum class Phase : std::uint8_t { Connecting, InWorld, Ended, Failed };

    [[nodiscard]] recording::Microseconds ElapsedAt(nexum::time::Seconds now) const noexcept;
    // The time of the record that ends the next group.
    [[nodiscard]] recording::Microseconds DueTimeOfNextGroup() const noexcept;
    void HandOverGroup(world::EventQueue& events);

    // Each record of a group; true when it ends the group.
    [[nodiscard]] bool Play(const recording::EnteredWorld& entry, world::EventQueue& events);
    [[nodiscard]] bool Play(const recording::WorldMessage& message, world::EventQueue& events);
    [[nodiscard]] bool Play(const recording::EpochChanged& change, world::EventQueue& events);
    [[nodiscard]] bool Play(const recording::TickMark& mark, world::EventQueue& events);
    [[nodiscard]] bool Play(const recording::Checkpoint& checkpoint, world::EventQueue& events);
    [[nodiscard]] bool Play(const recording::RecordingEnd& end, world::EventQueue& events);

    // False, and the replay failed, when the tick took another number of events.
    [[nodiscard]] bool CheckTick(const recording::TickMark& mark, std::size_t count);
    void CheckCheckpoint(const recording::Checkpoint& checkpoint, const world::WorldView& view);
    // The checkpoint right after the mark just checked, taken out of the records.
    [[nodiscard]] std::optional<recording::Checkpoint> TakeCheckpoint() noexcept;
    void Fail(const nexum::debug::LogLine& difference) noexcept;

    const recording::Recording* m_recording;
    nexum::debug::Logger* m_log;
    recording::Recording::Cursor m_cursor;
    // Born with the id of the hero, at the entry in the world.
    std::optional<translate::Translator> m_translator;
    session::SessionEpoch m_epoch;
    Phase m_phase = Phase::Connecting;
    bool m_started = false;
    nexum::time::Seconds m_start{0.0};
    // The group that comes next: the time of its last record, found once.
    std::optional<recording::Microseconds> m_due;
    // The events the group being handed over put in the queue.
    std::size_t m_handedOver = 0;
    // The mark of the tick the replay waits for.
    std::optional<recording::TickMark> m_waiting;
    std::size_t m_messages = 0;
    std::size_t m_ticks = 0;
    std::size_t m_checkpoints = 0;
    // The message of the difference, valid while the server lives.
    nexum::debug::LogLine m_failure;
};

} // namespace mu::online
