#include "Mu/Recording/Recorder.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "Mu/Recording/StateDigest.hpp"
#include "Nexum/Debug/LogLine.hpp"

namespace mu::recording {

using nexum::core::Result;
using nexum::time::Seconds;
using session::SessionEpoch;

namespace {

constexpr nexum::debug::LogCategory kRecording{"recording"};

// The counts of the records keep 4 bytes. A tick takes from the queue the events of a few frames, and a view
// keeps at most one object for each id of 2 bytes and one entry for each slot of 1 byte, so none comes near.
[[nodiscard]] std::uint32_t CountOf(std::size_t count) noexcept {
    return static_cast<std::uint32_t>(count);
}

} // namespace

Result<Recorder> Recorder::Create(const RecordingHeader& header, std::size_t room,
                                  nexum::debug::Logger& log) {
    auto writer = RecordingWriter::Create(header, room);
    if (!writer) {
        return tl::unexpected(std::move(writer.error()));
    }
    return Recorder(std::move(*writer), log);
}

Recorder::Recorder(RecordingWriter writer, nexum::debug::Logger& log) noexcept
    : m_writer(std::move(writer)), m_log(&log) {}

void Recorder::EnterWorld(Seconds now, SessionEpoch epoch, world::ObjectId hero) {
    if (m_inWorld) {
        return;
    }
    m_inWorld = true;
    m_entry = now;
    m_nextCheckpoint = now + kCheckpointPeriod;
    m_epoch = epoch;
    static_cast<void>(
        Put(EnteredWorld{.time = TimeOf(now), .epoch = epoch, .hero = hero}, Last::EnteredWorld, now));
}

void Recorder::NoteEpoch(Seconds now, SessionEpoch epoch) {
    if (!m_inWorld || epoch == m_epoch) {
        return;
    }
    m_epoch = epoch;
    static_cast<void>(Put(EpochChanged{.time = TimeOf(now), .epoch = epoch}, Last::EpochChanged, now));
}

void Recorder::NoteMessage(Seconds now, SessionEpoch epoch, std::span<const std::byte> message) {
    if (!m_inWorld) {
        return;
    }
    if (Put(WorldMessage{.time = TimeOf(now), .epoch = epoch, .bytes = message}, Last::Message, now)) {
        ++m_messages;
    }
}

void Recorder::NoteTick(const world::WorldView& view, std::size_t count, Seconds now) {
    if (!m_inWorld || !Put(TickMark{.time = TimeOf(now), .events = CountOf(count)}, Last::Tick, now)) {
        return;
    }
    ++m_ticks;
    if (now < m_nextCheckpoint) {
        return;
    }
    // The next one at the first point of the grid of 5 s after this tick, even after a long time without a
    // tick.
    while (m_nextCheckpoint <= now) {
        m_nextCheckpoint += kCheckpointPeriod;
    }
    PutCheckpoint(view, now);
}

void Recorder::Finish(const world::WorldView& view, Seconds now, EndReason reason) {
    if (m_writer.Ended()) {
        return;
    }
    if (m_last == Last::Tick) {
        PutCheckpoint(view, now);
    }
    const EndReason ending = m_writer.Full() ? EndReason::Full : reason;
    static_cast<void>(Put(RecordingEnd{.time = TimeOf(now), .reason = ending}, Last::End, now));
}

Microseconds Recorder::TimeOf(Seconds now) noexcept {
    if (m_inWorld) {
        // Rounded, because a time of the frame such as 5.6 s minus 0.7 s comes as 4.8999999... s.
        const auto since = std::chrono::round<std::chrono::microseconds>(now - m_entry).count();
        m_lastTime = std::max(m_lastTime, static_cast<Microseconds>(std::max<std::int64_t>(since, 0)));
    }
    return m_lastTime;
}

bool Recorder::Put(const Record& record, Last last, Seconds now) {
    const bool wasFull = m_writer.Full();
    if (m_writer.Write(record)) {
        m_last = last;
        return true;
    }
    if (m_writer.Full() && !wasFull) {
        nexum::debug::LogLine line;
        line.Append("the recording filled its room after ").AppendNumber(m_messages);
        line.Append(m_messages == 1 ? " message" : " messages").Append(", at ");
        line.AppendNumber(static_cast<std::uint64_t>(std::max(0.0, (now - m_entry).count())));
        line.Append(" s in the world: it stops here, and its end says it is full");
        m_log->Warning(kRecording, line.View());
    }
    return false;
}

void Recorder::PutCheckpoint(const world::WorldView& view, Seconds now) {
    const Checkpoint checkpoint{.time = TimeOf(now),
                                .messages = m_messages,
                                .digest = StateDigestOf(view),
                                .objects = CountOf(view.Objects().size()),
                                .items = CountOf(view.Inventory().size())};
    if (Put(checkpoint, Last::Checkpoint, now)) {
        ++m_checkpoints;
    }
}

} // namespace mu::recording
