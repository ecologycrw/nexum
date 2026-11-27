#pragma once

// The .mucap recording, version 1, which the Nexum defines itself (V.2.2 of the reference spec, ADR 0035, and
// the plan of slice 2, "O formato .mucap"): a header and then records, one after the other, all in
// little-endian. The writer puts them in a room reserved once, so recording never asks for memory (INV-13).
// The reader checks the whole recording before giving it, and refuses each invalid case with the exact reason
// (INV-12); the records are then read in order from the bytes, with nothing more to check.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Nexum/Core/Result.hpp"

namespace mu::recording {

// The versions of the format and of the digest of the state that this Nexum writes and reads.
inline constexpr std::uint16_t kFormatVersion = 1;
inline constexpr std::uint16_t kDigestVersion = 1;
// The longest text of the header, in bytes of UTF-8.
inline constexpr std::size_t kLongestText = 64;
// The largest recording that is read, 64 MiB, refused before its content is read.
inline constexpr std::size_t kLargestRecording = std::size_t{64} * 1024 * 1024;
// A message of the world as the session gives it: from the smallest packet, a C1 with only its header and
// code, to the largest C2 (docs/specs/protocolo/protocolo-106-3-m2.md, section 3).
inline constexpr std::size_t kShortestMessage = 3;
inline constexpr std::size_t kLongestMessage = 0xFFFF;

// The time of a record, in microseconds since the start of the recording, by the monotonic clock of the
// frame. It never goes back.
using Microseconds = std::uint64_t;

// The texts of the header. Each has at most 64 bytes.
struct RecordingHeader {
    // As `--version` gives it.
    std::string nexumVersion;
    // The version of the dialect, as the client gives it in the login (Mu/Protocol/Dialect.hpp).
    std::string dialectVersion;
    // The version of the server, as `--server-version` gives it, or "unknown".
    std::string server;
};

// The hero entered the world. At most one, before any message, change of epoch or tick.
struct EnteredWorld {
    Microseconds time = 0;
    session::SessionEpoch epoch;
    world::ObjectId hero;
};

// A message of the world, deciphered, as the session hands it to the translation.
struct WorldMessage {
    Microseconds time = 0;
    session::SessionEpoch epoch;
    // Valid while the recording that gave it lives.
    std::span<const std::byte> bytes;
};

// The epoch of the server changed.
struct EpochChanged {
    Microseconds time = 0;
    session::SessionEpoch epoch;
};

// A tick took events out of the queue: how many, applied or dropped for their epoch.
struct TickMark {
    Microseconds time = 0;
    std::uint32_t events = 0;
};

// The state of the view of the world right after a tick.
struct Checkpoint {
    Microseconds time = 0;
    // The messages recorded since the start.
    std::uint32_t messages = 0;
    // The digest of the state, in the version of the header.
    std::uint64_t digest = 0;
    std::uint32_t objects = 0;
    std::uint32_t items = 0;
};

enum class EndReason : std::uint8_t {
    // The client was closed, or the time asked in the world passed.
    Closed = 0,
    SessionFailed = 1,
    // The room of the recording filled up, and the recording stopped there.
    Full = 2,
};

// The last record, always there.
struct RecordingEnd {
    Microseconds time = 0;
    EndReason reason = EndReason::Closed;
};

using Record = std::variant<EnteredWorld, WorldMessage, EpochChanged, TickMark, Checkpoint, RecordingEnd>;

// Writes a recording in a room reserved once: nothing asks for memory after Create. The room always keeps
// space for the end, so a recording that fills up can still end.
class RecordingWriter {
public:
    // Reserves `room` bytes and writes the header. Refuses a text longer than 64 bytes, and a room too small
    // for the header and the end.
    [[nodiscard]] static nexum::core::Result<RecordingWriter> Create(const RecordingHeader& header,
                                                                     std::size_t room);

    // A copy would lose the reserved room, so the writer only moves.
    RecordingWriter(const RecordingWriter&) = delete;
    RecordingWriter& operator=(const RecordingWriter&) = delete;
    RecordingWriter(RecordingWriter&&) noexcept = default;
    RecordingWriter& operator=(RecordingWriter&&) noexcept = default;
    ~RecordingWriter() = default;

    // Puts the record at the end. False when it was not written:
    // - it did not fit in the room with the end after it, and the writer is full from then on;
    // - the writer is full, and takes nothing but the end;
    // - the recording already ended;
    // - a message has fewer than 3 or more than 65,535 bytes, which the session never gives.
    // The end is written once, full or not.
    bool Write(const Record& record);

    [[nodiscard]] bool Full() const noexcept { return m_full; }
    [[nodiscard]] bool Ended() const noexcept { return m_ended; }
    // The recording so far.
    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept { return m_bytes; }

private:
    explicit RecordingWriter(std::size_t room);

    // Makes the recording `size` bytes longer, inside the room, and gives the new bytes.
    [[nodiscard]] std::span<std::byte> Grow(std::size_t size) noexcept;

    std::vector<std::byte> m_bytes;
    std::size_t m_room;
    bool m_full = false;
    bool m_ended = false;
};

// A recording that passed every check of the reading.
class Recording {
public:
    // The records in order, read from the bytes of the recording, without asking for memory.
    class Cursor {
    public:
        [[nodiscard]] bool AtEnd() const noexcept { return m_offset >= m_bytes.size(); }
        // The next record. Only when not AtEnd.
        [[nodiscard]] Record Next() noexcept;

    private:
        friend class Recording;
        Cursor(std::span<const std::byte> bytes, std::size_t offset) noexcept
            : m_bytes(bytes), m_offset(offset) {}

        std::span<const std::byte> m_bytes;
        std::size_t m_offset;
    };

    Recording(const Recording&) = delete;
    Recording& operator=(const Recording&) = delete;
    // Moving keeps the bytes where they are, so the messages of a cursor stay valid.
    Recording(Recording&&) noexcept = default;
    Recording& operator=(Recording&&) noexcept = default;
    ~Recording() = default;

    [[nodiscard]] const RecordingHeader& Header() const noexcept { return m_header; }
    [[nodiscard]] Cursor Records() const noexcept { return Cursor(m_bytes, m_firstRecord); }

    // How many of each record the recording has.
    [[nodiscard]] std::size_t Messages() const noexcept { return m_messages; }
    [[nodiscard]] std::size_t Ticks() const noexcept { return m_ticks; }
    [[nodiscard]] std::size_t Checkpoints() const noexcept { return m_checkpoints; }
    [[nodiscard]] EndReason End() const noexcept { return m_end; }

private:
    friend nexum::core::Result<Recording> ParseRecording(std::vector<std::byte> bytes);
    Recording() = default;

    std::vector<std::byte> m_bytes;
    RecordingHeader m_header;
    std::size_t m_firstRecord = 0;
    std::size_t m_messages = 0;
    std::size_t m_ticks = 0;
    std::size_t m_checkpoints = 0;
    EndReason m_end = EndReason::Closed;
};

// Checks the whole recording, in this order: the size, the mark, the two versions and the texts of the
// header, then each record whole and of a known type and size, the time that never goes back, the entry in
// the world before the rest, each checkpoint right after a tick and with the right count of messages, and a
// single end as the last record. Gives the first reason it found.
[[nodiscard]] nexum::core::Result<Recording> ParseRecording(std::vector<std::byte> bytes);

// Reads the file and checks it with ParseRecording. A file larger than 64 MiB is refused before its content
// is read. The reading waits for the disk, so it never runs inside the frame (INV-06).
[[nodiscard]] nexum::core::Result<Recording> ReadRecording(const std::filesystem::path& path);

} // namespace mu::recording
