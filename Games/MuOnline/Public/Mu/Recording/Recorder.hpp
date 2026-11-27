#pragma once

// The recorder of a session in the world (ADR 0035, and the plan of slice 2, item 8). The live server tells
// it of the entry in the world, of each message of the world before the translation and of each change of
// epoch; after each tick that took events, the live server passes on the mark of the tick, and every 5 s in
// the world the recorder adds a checkpoint with the digest of the view (Mu/Recording/StateDigest.hpp). It
// writes them as the records of a .mucap recording (Mu/Recording/Recording.hpp), in a room reserved once, so
// recording never asks for memory (INV-13); the client writes the file only when it ends, outside the frames
// (INV-06).
//
// It starts only at the entry in the world, and its clock with it: the time of each record counts from there.
// It never sees what the client sends, nor the list of servers, the login and the list of characters, so the
// account and the password never reach the file (INV-18).

#include <cstddef>
#include <cstdint>
#include <span>

#include "Mu/Recording/Recording.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::recording {

// The room of the MuClient: 16 MiB (decision 4 of the spec of slice 2).
inline constexpr std::size_t kRecorderRoom = std::size_t{16} * 1024 * 1024;
// A checkpoint every 5 s in the world (CA-04 of slice 2).
inline constexpr nexum::time::Seconds kCheckpointPeriod{5.0};

class Recorder {
public:
    // Reserves `room` bytes and writes the header. Refuses as the writer does: a text longer than 64 bytes,
    // or a room too small for the header and the end. The log must outlive the recorder.
    [[nodiscard]] static nexum::core::Result<Recorder> Create(const RecordingHeader& header, std::size_t room,
                                                              nexum::debug::Logger& log);

    // The hero entered the world: the recording starts here, at the time 0. Only the first call counts.
    void EnterWorld(nexum::time::Seconds now, session::SessionEpoch epoch, world::ObjectId hero);

    // The epoch of the server now; a change since the last one is recorded.
    void NoteEpoch(nexum::time::Seconds now, session::SessionEpoch epoch);

    // A message of the world, deciphered, as the session hands it to the translation.
    void NoteMessage(nexum::time::Seconds now, session::SessionEpoch epoch,
                     std::span<const std::byte> message);

    // A tick took `count` events out of the queue, applied or dropped, and left the view so: the mark of the
    // tick, and then a checkpoint at the first tick from each 5 s in the world on.
    void NoteTick(const world::WorldView& view, std::size_t count, nexum::time::Seconds now);

    // The end, once, as the last record. Right after the mark of a tick it adds the checkpoint of the end
    // first, unless that tick already has one; after a message or a change of epoch, a checkpoint cannot
    // come, because one follows only a tick. So the client applies the events that wait before it ends. A
    // recording whose room filled up ends as full, whatever the reason given.
    void Finish(const world::WorldView& view, nexum::time::Seconds now, EndReason reason);

    // The recording so far.
    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept { return m_writer.Bytes(); }
    [[nodiscard]] std::uint32_t Messages() const noexcept { return m_messages; }
    [[nodiscard]] std::uint32_t Ticks() const noexcept { return m_ticks; }
    [[nodiscard]] std::uint32_t Checkpoints() const noexcept { return m_checkpoints; }
    [[nodiscard]] bool Full() const noexcept { return m_writer.Full(); }
    [[nodiscard]] bool Ended() const noexcept { return m_writer.Ended(); }

private:
    // The record written last, which tells where a checkpoint may come.
    enum class Last : std::uint8_t { Header, EnteredWorld, Message, EpochChanged, Tick, Checkpoint, End };

    Recorder(RecordingWriter writer, nexum::debug::Logger& log) noexcept;

    // The time of a record: the microseconds since the entry in the world, never less than the one before.
    [[nodiscard]] Microseconds TimeOf(nexum::time::Seconds now) noexcept;
    // Writes the record; when it does not fit, the recording stops there, with a warning, once.
    [[nodiscard]] bool Put(const Record& record, Last last, nexum::time::Seconds now);
    void PutCheckpoint(const world::WorldView& view, nexum::time::Seconds now);

    RecordingWriter m_writer;
    nexum::debug::Logger* m_log;
    bool m_inWorld = false;
    nexum::time::Seconds m_entry{0.0};
    nexum::time::Seconds m_nextCheckpoint{0.0};
    Microseconds m_lastTime = 0;
    session::SessionEpoch m_epoch;
    Last m_last = Last::Header;
    std::uint32_t m_messages = 0;
    std::uint32_t m_ticks = 0;
    std::uint32_t m_checkpoints = 0;
};

} // namespace mu::recording
