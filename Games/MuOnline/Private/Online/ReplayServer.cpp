#include "Mu/Online/ReplayServer.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "Mu/Recording/StateDigest.hpp"

namespace mu::online {

using nexum::debug::LogLine;
using nexum::time::Seconds;
using recording::Checkpoint;
using recording::EndReason;
using recording::EnteredWorld;
using recording::EpochChanged;
using recording::Microseconds;
using recording::Record;
using recording::RecordingEnd;
using recording::TickMark;
using recording::WorldMessage;

namespace {

constexpr nexum::debug::LogCategory kReplay{"replay"};
// The digest is shown with all its 16 hexadecimal digits, and an id with its 4.
constexpr std::size_t kDigestDigits = 16;
constexpr std::size_t kIdDigits = 4;

// "1 tick", "2 ticks".
LogLine& AppendCount(LogLine& line, std::uint64_t count, std::string_view thing) noexcept {
    line.AppendNumber(count).Append(" ").Append(thing);
    if (count != 1) {
        line.Append("s");
    }
    return line;
}

[[nodiscard]] std::string CountText(std::size_t count, std::string_view thing) {
    std::string text = std::to_string(count) + " " + std::string(thing);
    if (count != 1) {
        text += "s";
    }
    return text;
}

[[nodiscard]] std::string_view HowItEnded(EndReason reason) noexcept {
    switch (reason) {
    case EndReason::Closed:
        return "the client ended it";
    case EndReason::SessionFailed:
        return "the session failed";
    case EndReason::Full:
        return "its room filled up";
    }
    return "the client ended it";
}

[[nodiscard]] Microseconds TimeOf(const Record& record) noexcept {
    return std::visit([](const auto& any) { return any.time; }, record);
}

// The state of the view as a checkpoint keeps it.
void AppendState(LogLine& line, std::uint64_t digest, std::uint64_t objects, std::uint64_t items) noexcept {
    line.Append("the digest ").AppendHex(digest, kDigestDigits).Append(" with ");
    AppendCount(line, objects, "object").Append(" and ");
    AppendCount(line, items, "item");
}

} // namespace

ReplayServer::ReplayServer(const recording::Recording& recording, nexum::debug::Logger& log)
    : m_recording(&recording), m_log(&log), m_cursor(recording.Records()) {}

void ReplayServer::Start(Seconds now) {
    m_started = true;
    m_start = now;
    LogLine line;
    line.Append("replaying ");
    AppendCount(line, m_recording->Messages(), "message").Append(", ");
    AppendCount(line, m_recording->Ticks(), "tick").Append(" and ");
    AppendCount(line, m_recording->Checkpoints(), "checkpoint");
    line.Append(", recorded by the Nexum ").Append(m_recording->Header().nexumVersion);
    line.Append(" with the server ").Append(m_recording->Header().server);
    m_log->Info(kReplay, line.View());
}

void ReplayServer::TakeEvents(Seconds now, world::EventQueue& events) {
    if (!m_started) {
        return;
    }
    const Microseconds elapsed = ElapsedAt(now);
    // A group whose messages give no events is checked at once, so the next one may come in the same frame.
    while ((m_phase == Phase::Connecting || m_phase == Phase::InWorld) && !m_waiting) {
        if (!m_due) {
            m_due = DueTimeOfNextGroup();
        }
        if (*m_due > elapsed) {
            return;
        }
        m_due.reset();
        HandOverGroup(events);
    }
}

Microseconds ReplayServer::ElapsedAt(Seconds now) const noexcept {
    // Rounded as the recorder rounds the times it writes.
    const auto since = std::chrono::round<std::chrono::microseconds>(now - m_start).count();
    return static_cast<Microseconds>(std::max<std::int64_t>(since, 0));
}

Microseconds ReplayServer::DueTimeOfNextGroup() const noexcept {
    // The reading checked the recording: it ends with the end, and a group never runs past it.
    for (auto ahead = m_cursor; !ahead.AtEnd();) {
        const Record record = ahead.Next();
        if (std::holds_alternative<EnteredWorld>(record) || std::holds_alternative<TickMark>(record) ||
            std::holds_alternative<RecordingEnd>(record)) {
            return TimeOf(record);
        }
    }
    return 0;
}

void ReplayServer::HandOverGroup(world::EventQueue& events) {
    m_handedOver = 0;
    while (!m_cursor.AtEnd()) {
        const Record record = m_cursor.Next();
        if (std::visit([this, &events](const auto& any) { return Play(any, events); }, record)) {
            return;
        }
    }
    // Only a recording without its end, which the reading refuses, gets here.
    m_phase = Phase::Ended;
}

bool ReplayServer::Play(const EnteredWorld& entry, world::EventQueue& /*events*/) {
    m_translator.emplace(entry.hero, *m_log);
    m_epoch = entry.epoch;
    m_phase = Phase::InWorld;
    LogLine line;
    line.Append("the replay is in the world, with the hero ").AppendHex(entry.hero.value, kIdDigits);
    m_log->Info(kReplay, line.View());
    return true;
}

bool ReplayServer::Play(const WorldMessage& message, world::EventQueue& events) {
    // The reading puts the entry in the world before any message, so the translation is always there.
    if (m_translator) {
        m_handedOver += m_translator->Translate(message.epoch, message.bytes, events);
    }
    ++m_messages;
    return false;
}

bool ReplayServer::Play(const EpochChanged& change, world::EventQueue& /*events*/) {
    m_epoch = change.epoch;
    return false;
}

bool ReplayServer::Play(const TickMark& mark, world::EventQueue& /*events*/) {
    if (m_handedOver > 0) {
        m_waiting = mark;
        return true;
    }
    // No tick of the client will take an event, so none reaches the server: the tick is checked now.
    if (!CheckTick(mark, 0)) {
        return true;
    }
    if (const auto checkpoint = TakeCheckpoint()) {
        LogLine difference;
        difference.Append("the checkpoint ").AppendNumber(m_checkpoints + 1);
        difference.Append(" at ")
            .AppendNumber(checkpoint->time)
            .Append(" us follows a tick without events, ");
        difference.Append("which never reaches the replay, so the view cannot be compared");
        Fail(difference);
    }
    return true;
}

// A checkpoint comes only right after a tick, which takes it (TakeCheckpoint), so none starts a group.
bool ReplayServer::Play(const Checkpoint& /*checkpoint*/, world::EventQueue& /*events*/) {
    return false;
}

bool ReplayServer::Play(const RecordingEnd& end, world::EventQueue& /*events*/) {
    m_phase = Phase::Ended;
    LogLine line;
    line.Append("the replay reached the end of the recording, where ").Append(HowItEnded(end.reason));
    line.Append(": every tick and checkpoint matched");
    m_log->Info(kReplay, line.View());
    return true;
}

void ReplayServer::EventsApplied(const world::WorldView& view, std::size_t count, Seconds /*now*/) {
    if (!m_waiting || m_phase != Phase::InWorld) {
        return;
    }
    const TickMark mark = *m_waiting;
    m_waiting.reset();
    if (!CheckTick(mark, count)) {
        return;
    }
    if (const auto checkpoint = TakeCheckpoint()) {
        CheckCheckpoint(*checkpoint, view);
    }
}

bool ReplayServer::CheckTick(const TickMark& mark, std::size_t count) {
    if (count == mark.events) {
        ++m_ticks;
        return true;
    }
    LogLine difference;
    difference.Append("the tick ").AppendNumber(m_ticks + 1).Append(" at ").AppendNumber(mark.time);
    difference.Append(" us took ");
    AppendCount(difference, mark.events, "event").Append(" in the recording, but ");
    difference.AppendNumber(count).Append(" in the replay");
    Fail(difference);
    return false;
}

void ReplayServer::CheckCheckpoint(const Checkpoint& checkpoint, const world::WorldView& view) {
    const std::uint64_t digest = recording::StateDigestOf(view);
    const std::size_t objects = view.Objects().size();
    const std::size_t items = view.Inventory().size();
    if (digest == checkpoint.digest && objects == checkpoint.objects && items == checkpoint.items) {
        ++m_checkpoints;
        return;
    }
    LogLine difference;
    difference.Append("the checkpoint ").AppendNumber(m_checkpoints + 1);
    difference.Append(" at ").AppendNumber(checkpoint.time).Append(" us has ");
    AppendState(difference, checkpoint.digest, checkpoint.objects, checkpoint.items);
    difference.Append(" in the recording, but ");
    AppendState(difference, digest, objects, items);
    difference.Append(" in the replay");
    Fail(difference);
}

std::optional<Checkpoint> ReplayServer::TakeCheckpoint() noexcept {
    auto ahead = m_cursor;
    if (ahead.AtEnd()) {
        return std::nullopt;
    }
    const Record record = ahead.Next();
    const auto* checkpoint = std::get_if<Checkpoint>(&record);
    if (checkpoint == nullptr) {
        return std::nullopt;
    }
    m_cursor = ahead;
    return *checkpoint;
}

void ReplayServer::Fail(const LogLine& difference) noexcept {
    m_failure = difference;
    m_phase = Phase::Failed;
    m_log->Error(kReplay, m_failure.View());
}

ServerStatus ReplayServer::Status() const noexcept {
    switch (m_phase) {
    case Phase::Connecting:
        break;
    case Phase::InWorld:
        return ServerStatus{.state = ServerState::InWorld, .reason = {}};
    case Phase::Ended:
        return ServerStatus{.state = ServerState::Ended, .reason = {}};
    case Phase::Failed:
        return ServerStatus{.state = ServerState::Failed, .reason = m_failure.View()};
    }
    return ServerStatus{};
}

std::string ReplayServer::Summary(Seconds /*now*/) const {
    return "the replay checked " + CountText(m_messages, "message") + ", " + CountText(m_ticks, "tick") +
           " and " + CountText(m_checkpoints, "checkpoint");
}

} // namespace mu::online
