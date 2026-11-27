#include "Mu/Recording/Recording.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "Mu/Protocol/PacketFields.hpp"
#include "Nexum/Platform/FileBytes.hpp"

namespace mu::recording {

using nexum::core::MakeError;
using nexum::core::Result;
using protocol::ReadByte;
using protocol::ReadLittleEndian;
using protocol::WriteByte;
using protocol::WriteLittleEndian;
using session::SessionEpoch;

namespace {

// "NXMUCAP" and the byte 1A, which stops a text reader that opens the file by mistake.
constexpr std::array kMark{std::byte{0x4E}, std::byte{0x58}, std::byte{0x4D}, std::byte{0x55},
                           std::byte{0x43}, std::byte{0x41}, std::byte{0x50}, std::byte{0x1A}};
// The mark and the two versions, before the texts.
constexpr std::size_t kVersionsEnd = kMark.size() + 4;

// The byte that starts each record.
enum class RecordType : std::uint8_t {
    EnteredWorld = 1,
    Message = 2,
    EpochChanged = 3,
    Tick = 4,
    Checkpoint = 5,
    End = 6,
};

// Each record has the type (1 byte), the size of the content (4) and the content, which starts with the time
// (8).
constexpr std::size_t kRecordFrame = 5;
constexpr std::size_t kAfterTime = 8;
constexpr std::size_t kEnteredWorldSize = 14; // time, epoch (4), hero (2)
constexpr std::size_t kMessageFixedSize = 12; // time, epoch (4), then the message
constexpr std::size_t kEpochChangedSize = 12; // time, epoch (4)
constexpr std::size_t kTickSize = 12;         // time, events (4)
constexpr std::size_t kCheckpointSize = 28;   // time, messages (4), digest (8), objects (4), items (4)
constexpr std::size_t kEndSize = 9;           // time, reason (1)
constexpr std::size_t kEndRecord = kRecordFrame + kEndSize;
constexpr std::uint8_t kLastEndReason = static_cast<std::uint8_t>(EndReason::Full);

// What the reading knows of each type: its name in the reasons and the sizes its content may have.
struct RecordShape {
    RecordType type;
    std::string_view name;
    std::size_t shortest;
    std::size_t longest;
};

constexpr std::array kShapes{
    RecordShape{RecordType::EnteredWorld, "entry in the world", kEnteredWorldSize, kEnteredWorldSize},
    RecordShape{RecordType::Message, "message", kMessageFixedSize + kShortestMessage,
                kMessageFixedSize + kLongestMessage},
    RecordShape{RecordType::EpochChanged, "change of epoch", kEpochChangedSize, kEpochChangedSize},
    RecordShape{RecordType::Tick, "tick", kTickSize, kTickSize},
    RecordShape{RecordType::Checkpoint, "checkpoint", kCheckpointSize, kCheckpointSize},
    RecordShape{RecordType::End, "end", kEndSize, kEndSize},
};

[[nodiscard]] const RecordShape* FindShape(std::uint8_t type) noexcept {
    const auto found = std::ranges::find(
        kShapes, type, [](const RecordShape& shape) { return static_cast<std::uint8_t>(shape.type); });
    return found == kShapes.end() ? nullptr : &*found;
}

[[nodiscard]] const RecordShape& ShapeOf(RecordType type) noexcept {
    return *FindShape(static_cast<std::uint8_t>(type));
}

[[nodiscard]] std::string At(std::string_view name, std::size_t offset) {
    return "the " + std::string(name) + " at byte " + std::to_string(offset);
}

// The texts of the header, in their order, with their names in the reasons.
struct HeaderText {
    std::string_view name;
    std::string RecordingHeader::* field;
};

constexpr std::array kHeaderTexts{
    HeaderText{"Nexum version", &RecordingHeader::nexumVersion},
    HeaderText{"dialect version", &RecordingHeader::dialectVersion},
    HeaderText{"server", &RecordingHeader::server},
};

[[nodiscard]] nexum::core::Error TextTooLong(std::string_view name, std::size_t size) {
    return {"the " + std::string(name) + " in the header has " + std::to_string(size) + " bytes, more than " +
            std::to_string(kLongestText)};
}

// The texts of the header are UTF-8 without control characters, because the replay shows them in its first
// line, and a control character there would reach the terminal (INV-12).
//
// RFC 3629, section 4: the first byte of a character of 2 to 4 bytes, and the range of its second byte, which
// keeps out the longer forms of a shorter character, the surrogates and what is past U+10FFFF. Every other
// byte after the first is from 80 to BF.
struct Utf8Lead {
    std::uint8_t first;
    std::uint8_t last;
    std::size_t length;
    std::uint8_t lowestSecond;
    std::uint8_t highestSecond;
};

constexpr std::array kUtf8Leads{
    Utf8Lead{0xC2, 0xDF, 2, 0x80, 0xBF}, Utf8Lead{0xE0, 0xE0, 3, 0xA0, 0xBF},
    Utf8Lead{0xE1, 0xEC, 3, 0x80, 0xBF}, Utf8Lead{0xED, 0xED, 3, 0x80, 0x9F},
    Utf8Lead{0xEE, 0xEF, 3, 0x80, 0xBF}, Utf8Lead{0xF0, 0xF0, 4, 0x90, 0xBF},
    Utf8Lead{0xF1, 0xF3, 4, 0x80, 0xBF}, Utf8Lead{0xF4, 0xF4, 4, 0x80, 0x8F},
};
constexpr std::uint8_t kLowestNext = 0x80;
constexpr std::uint8_t kHighestNext = 0xBF;
// The control characters: C0 (00 to 1F) and DEL (7F), of one byte, and C1 (U+0080 to U+009F), C2 80 to C2 9F.
constexpr std::uint8_t kFirstShown = 0x20;
constexpr std::uint8_t kDelete = 0x7F;
constexpr std::uint8_t kC1Lead = 0xC2;
constexpr std::uint8_t kLastC1Second = 0x9F;

// Where a text stops being UTF-8 without control characters, and why.
struct TextFault {
    std::size_t at = 0;
    bool control = false;
};

[[nodiscard]] bool IsNext(std::byte unit) noexcept {
    const auto value = std::to_integer<std::uint8_t>(unit);
    return value >= kLowestNext && value <= kHighestNext;
}

[[nodiscard]] std::optional<TextFault> FaultOf(std::span<const std::byte> text) noexcept {
    for (std::size_t at = 0; at < text.size();) {
        const auto first = std::to_integer<std::uint8_t>(text[at]);
        if (first < kFirstShown || first == kDelete) {
            return TextFault{.at = at, .control = true};
        }
        if (first < kDelete) {
            ++at;
            continue;
        }
        const auto lead = std::ranges::find_if(
            kUtf8Leads, [first](const Utf8Lead& one) { return first >= one.first && first <= one.last; });
        if (lead == kUtf8Leads.end() || lead->length > text.size() - at) {
            return TextFault{.at = at, .control = false};
        }
        const auto second = std::to_integer<std::uint8_t>(text[at + 1]);
        if (second < lead->lowestSecond || second > lead->highestSecond ||
            !std::all_of(text.begin() + static_cast<std::ptrdiff_t>(at + 2),
                         text.begin() + static_cast<std::ptrdiff_t>(at + lead->length), IsNext)) {
            return TextFault{.at = at, .control = false};
        }
        if (first == kC1Lead && second <= kLastC1Second) {
            return TextFault{.at = at, .control = true};
        }
        at += lead->length;
    }
    return std::nullopt;
}

// A text of the header that starts at the byte `start` of the recording; the reason says at which byte of
// the recording it went wrong, for the writer as for the reading.
[[nodiscard]] Result<void> CheckText(std::string_view name, std::span<const std::byte> text,
                                     std::size_t start) {
    const auto fault = FaultOf(text);
    if (!fault) {
        return {};
    }
    return MakeError("the " + std::string(name) + " in the header " +
                     (fault->control ? "has a control character" : "is not UTF-8") + " at byte " +
                     std::to_string(start + fault->at));
}

// Writing.

// The content of a record before it goes into the room: the fixed part, and the message for a message.
struct Encoded {
    RecordType type = RecordType::End;
    std::array<std::byte, kCheckpointSize> fixed{};
    std::size_t fixedSize = 0;
    std::span<const std::byte> message;
};

[[nodiscard]] Encoded StartEncoded(RecordType type, std::size_t fixedSize, Microseconds time) noexcept {
    Encoded encoded{.type = type, .fixed = {}, .fixedSize = fixedSize, .message = {}};
    WriteLittleEndian(encoded.fixed, 0, time);
    return encoded;
}

[[nodiscard]] Encoded Encode(const EnteredWorld& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::EnteredWorld, kEnteredWorldSize, record.time);
    WriteLittleEndian(encoded.fixed, kAfterTime, record.epoch.value);
    WriteLittleEndian(encoded.fixed, kAfterTime + 4, record.hero.value);
    return encoded;
}

[[nodiscard]] Encoded Encode(const WorldMessage& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::Message, kMessageFixedSize, record.time);
    WriteLittleEndian(encoded.fixed, kAfterTime, record.epoch.value);
    encoded.message = record.bytes;
    return encoded;
}

[[nodiscard]] Encoded Encode(const EpochChanged& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::EpochChanged, kEpochChangedSize, record.time);
    WriteLittleEndian(encoded.fixed, kAfterTime, record.epoch.value);
    return encoded;
}

[[nodiscard]] Encoded Encode(const TickMark& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::Tick, kTickSize, record.time);
    WriteLittleEndian(encoded.fixed, kAfterTime, record.events);
    return encoded;
}

[[nodiscard]] Encoded Encode(const Checkpoint& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::Checkpoint, kCheckpointSize, record.time);
    WriteLittleEndian(encoded.fixed, kAfterTime, record.messages);
    WriteLittleEndian(encoded.fixed, kAfterTime + 4, record.digest);
    WriteLittleEndian(encoded.fixed, kAfterTime + 12, record.objects);
    WriteLittleEndian(encoded.fixed, kAfterTime + 16, record.items);
    return encoded;
}

[[nodiscard]] Encoded Encode(const RecordingEnd& record) noexcept {
    Encoded encoded = StartEncoded(RecordType::End, kEndSize, record.time);
    WriteByte(encoded.fixed, kAfterTime, static_cast<std::uint8_t>(record.reason));
    return encoded;
}

// False only for a message with fewer than 3 or more than 65,535 bytes.
[[nodiscard]] bool FitsTheFormat(const Record& record) noexcept {
    const auto* message = std::get_if<WorldMessage>(&record);
    return message == nullptr ||
           (message->bytes.size() >= kShortestMessage && message->bytes.size() <= kLongestMessage);
}

// Reading.

[[nodiscard]] SessionEpoch EpochAt(std::span<const std::byte> content) noexcept {
    return SessionEpoch{ReadLittleEndian<std::uint32_t>(content, kAfterTime)};
}

// The record of a content whose size the reading already checked.
[[nodiscard]] Record Decode(RecordType type, std::span<const std::byte> content) noexcept {
    const Microseconds time = ReadLittleEndian<std::uint64_t>(content, 0);
    switch (type) {
    case RecordType::EnteredWorld:
        return EnteredWorld{.time = time,
                            .epoch = EpochAt(content),
                            .hero =
                                world::ObjectId{ReadLittleEndian<std::uint16_t>(content, kAfterTime + 4)}};
    case RecordType::Message:
        return WorldMessage{
            .time = time, .epoch = EpochAt(content), .bytes = content.subspan(kMessageFixedSize)};
    case RecordType::EpochChanged:
        return EpochChanged{.time = time, .epoch = EpochAt(content)};
    case RecordType::Tick:
        return TickMark{.time = time, .events = ReadLittleEndian<std::uint32_t>(content, kAfterTime)};
    case RecordType::Checkpoint:
        return Checkpoint{.time = time,
                          .messages = ReadLittleEndian<std::uint32_t>(content, kAfterTime),
                          .digest = ReadLittleEndian<std::uint64_t>(content, kAfterTime + 4),
                          .objects = ReadLittleEndian<std::uint32_t>(content, kAfterTime + 12),
                          .items = ReadLittleEndian<std::uint32_t>(content, kAfterTime + 16)};
    case RecordType::End:
        break;
    }
    return RecordingEnd{.time = time, .reason = static_cast<EndReason>(ReadByte(content, kAfterTime))};
}

// A text of the header: a byte with the size, then the bytes.
[[nodiscard]] Result<std::string> ReadText(std::span<const std::byte> bytes, std::size_t& offset,
                                           std::string_view name) {
    if (offset >= bytes.size()) {
        return MakeError("the header is cut");
    }
    const std::size_t size = ReadByte(bytes, offset);
    if (size > kLongestText) {
        return tl::unexpected(TextTooLong(name, size));
    }
    if (size > bytes.size() - offset - 1) {
        return MakeError("the header is cut");
    }
    const auto text = bytes.subspan(offset + 1, size);
    if (auto shown = CheckText(name, text, offset + 1); !shown) {
        return tl::unexpected(shown.error());
    }
    offset += 1 + size;
    std::string read(size, '\0');
    std::ranges::transform(text, read.begin(), [](std::byte unit) { return static_cast<char>(unit); });
    return read;
}

[[nodiscard]] Result<void> CheckVersion(std::uint16_t version, std::string_view what, std::uint16_t known) {
    if (version != known) {
        return MakeError("the " + std::string(what) + " version " + std::to_string(version) +
                         " is unknown: this Nexum reads the version " + std::to_string(known));
    }
    return {};
}

// The header, from the mark to the last text. Gives where the records start.
[[nodiscard]] Result<std::size_t> ReadHeader(std::span<const std::byte> bytes, RecordingHeader& header) {
    if (bytes.size() < kMark.size() || !std::ranges::equal(bytes.first(kMark.size()), kMark)) {
        return MakeError("the recording does not start with the mark of a .mucap");
    }
    if (bytes.size() < kVersionsEnd) {
        return MakeError("the header is cut");
    }
    if (auto format =
            CheckVersion(ReadLittleEndian<std::uint16_t>(bytes, kMark.size()), "format", kFormatVersion);
        !format) {
        return tl::unexpected(format.error());
    }
    if (auto digest =
            CheckVersion(ReadLittleEndian<std::uint16_t>(bytes, kMark.size() + 2), "digest", kDigestVersion);
        !digest) {
        return tl::unexpected(digest.error());
    }
    std::size_t offset = kVersionsEnd;
    for (const HeaderText& text : kHeaderTexts) {
        auto read = ReadText(bytes, offset, text.name);
        if (!read) {
            return tl::unexpected(read.error());
        }
        header.*text.field = std::move(*read);
    }
    return offset;
}

// One record, whole inside the recording, of a known type and of a size of that type.
struct Frame {
    const RecordShape* shape = nullptr;
    std::span<const std::byte> content;
};

[[nodiscard]] Result<Frame> ReadFrame(std::span<const std::byte> bytes, std::size_t offset) {
    const std::size_t left = bytes.size() - offset;
    if (left < kRecordFrame) {
        return MakeError("the record at byte " + std::to_string(offset) + " is cut");
    }
    const std::uint8_t type = ReadByte(bytes, offset);
    const RecordShape* shape = FindShape(type);
    if (shape == nullptr) {
        return MakeError("the record at byte " + std::to_string(offset) + " has the unknown type " +
                         std::to_string(type));
    }
    const std::uint32_t size = ReadLittleEndian<std::uint32_t>(bytes, offset + 1);
    if (size < shape->shortest || size > shape->longest) {
        const std::string expected =
            shape->shortest == shape->longest
                ? std::to_string(shape->shortest)
                : std::to_string(shape->shortest) + " to " + std::to_string(shape->longest);
        return MakeError(At(shape->name, offset) + " has " + std::to_string(size) +
                         " bytes of content, not " + expected);
    }
    if (size > left - kRecordFrame) {
        return MakeError("the record at byte " + std::to_string(offset) + " is cut");
    }
    const auto content = bytes.subspan(offset + kRecordFrame, size);
    if (shape->type == RecordType::End && ReadByte(content, kAfterTime) > kLastEndReason) {
        return MakeError(At(shape->name, offset) + " has the unknown reason " +
                         std::to_string(ReadByte(content, kAfterTime)));
    }
    return Frame{.shape = shape, .content = content};
}

// The rules between the records, checked one record at a time, in the order of the reading.
class SequenceCheck {
public:
    [[nodiscard]] Result<void> Check(const RecordShape& shape, const Record& record, std::size_t offset) {
        const Microseconds time = std::visit([](const auto& any) { return any.time; }, record);
        if (time < m_lastTime) {
            return MakeError(At(shape.name, offset) + " goes back in time: " + std::to_string(time) +
                             " us after " + std::to_string(m_lastTime) + " us");
        }
        if (auto order = CheckOrder(shape, record, offset); !order) {
            return order;
        }
        m_lastTime = time;
        m_afterTick = shape.type == RecordType::Tick;
        return {};
    }

    // After the last record.
    [[nodiscard]] Result<void> Finish() const {
        if (!m_ended) {
            return MakeError("the recording has no end");
        }
        return {};
    }

    [[nodiscard]] std::size_t Messages() const noexcept { return m_messages; }
    [[nodiscard]] std::size_t Ticks() const noexcept { return m_ticks; }
    [[nodiscard]] std::size_t Checkpoints() const noexcept { return m_checkpoints; }
    [[nodiscard]] EndReason End() const noexcept { return m_end; }

private:
    [[nodiscard]] Result<void> CheckOrder(const RecordShape& shape, const Record& record,
                                          std::size_t offset) {
        if (m_ended) {
            return MakeError(At(shape.name, offset) + " comes after the end");
        }
        switch (shape.type) {
        case RecordType::EnteredWorld:
            if (m_entered) {
                return MakeError(At(shape.name, offset) + " is the second one");
            }
            m_entered = true;
            return {};
        case RecordType::Message:
        case RecordType::EpochChanged:
        case RecordType::Tick:
            return CheckInWorld(shape, offset);
        case RecordType::Checkpoint:
            return CheckCheckpoint(std::get<Checkpoint>(record), offset);
        case RecordType::End:
            m_ended = true;
            m_end = std::get<RecordingEnd>(record).reason;
            return {};
        }
        return {};
    }

    [[nodiscard]] Result<void> CheckInWorld(const RecordShape& shape, std::size_t offset) {
        if (!m_entered) {
            return MakeError(At(shape.name, offset) + " comes before the entry in the world");
        }
        if (shape.type == RecordType::Message) {
            ++m_messages;
        } else if (shape.type == RecordType::Tick) {
            ++m_ticks;
        }
        return {};
    }

    [[nodiscard]] Result<void> CheckCheckpoint(const Checkpoint& checkpoint, std::size_t offset) {
        const std::string at = At(ShapeOf(RecordType::Checkpoint).name, offset);
        if (!m_afterTick) {
            return MakeError(at + " does not come right after a tick");
        }
        if (checkpoint.messages != m_messages) {
            return MakeError(at + " counts " + std::to_string(checkpoint.messages) +
                             " messages, but the recording has " + std::to_string(m_messages) + " before it");
        }
        ++m_checkpoints;
        return {};
    }

    Microseconds m_lastTime = 0;
    bool m_entered = false;
    bool m_afterTick = false;
    bool m_ended = false;
    std::size_t m_messages = 0;
    std::size_t m_ticks = 0;
    std::size_t m_checkpoints = 0;
    EndReason m_end = EndReason::Closed;
};

} // namespace

// The writer.

RecordingWriter::RecordingWriter(std::size_t room) : m_room(room) {
    m_bytes.reserve(room);
}

Result<RecordingWriter> RecordingWriter::Create(const RecordingHeader& header, std::size_t room) {
    std::size_t headerSize = kVersionsEnd;
    for (const HeaderText& text : kHeaderTexts) {
        const std::string& value = header.*text.field;
        const std::size_t size = value.size();
        if (size > kLongestText) {
            return tl::unexpected(TextTooLong(text.name, size));
        }
        if (auto shown = CheckText(text.name, std::as_bytes(std::span(value)), headerSize + 1); !shown) {
            return tl::unexpected(shown.error());
        }
        headerSize += 1 + size;
    }
    if (room < headerSize + kEndRecord) {
        return MakeError("the room of " + std::to_string(room) +
                         " bytes is too small for the header and the end, " + "which take " +
                         std::to_string(headerSize + kEndRecord));
    }

    RecordingWriter writer(room);
    const std::span<std::byte> bytes = writer.Grow(headerSize);
    std::ranges::copy(kMark, bytes.begin());
    WriteLittleEndian(bytes, kMark.size(), kFormatVersion);
    WriteLittleEndian(bytes, kMark.size() + 2, kDigestVersion);
    std::size_t offset = kVersionsEnd;
    for (const HeaderText& text : kHeaderTexts) {
        const std::string& value = header.*text.field;
        WriteByte(bytes, offset, static_cast<std::uint8_t>(value.size()));
        std::ranges::transform(value, bytes.subspan(offset + 1).begin(),
                               [](char unit) { return static_cast<std::byte>(unit); });
        offset += 1 + value.size();
    }
    return writer;
}

bool RecordingWriter::Write(const Record& record) {
    const bool isEnd = std::holds_alternative<RecordingEnd>(record);
    if (m_ended || (m_full && !isEnd) || !FitsTheFormat(record)) {
        return false;
    }
    const Encoded encoded = std::visit([](const auto& any) { return Encode(any); }, record);
    const std::size_t content = encoded.fixedSize + encoded.message.size();
    // The room always keeps the end, so the end itself always fits.
    const std::size_t needed = kRecordFrame + content + (isEnd ? 0 : kEndRecord);
    if (needed > m_room - m_bytes.size()) {
        m_full = true;
        return false;
    }

    const std::span<std::byte> bytes = Grow(kRecordFrame + content);
    WriteByte(bytes, 0, static_cast<std::uint8_t>(encoded.type));
    WriteLittleEndian(bytes, 1, static_cast<std::uint32_t>(content));
    std::ranges::copy(std::span(encoded.fixed).first(encoded.fixedSize), bytes.subspan(kRecordFrame).begin());
    std::ranges::copy(encoded.message, bytes.subspan(kRecordFrame + encoded.fixedSize).begin());
    m_ended = isEnd;
    return true;
}

std::span<std::byte> RecordingWriter::Grow(std::size_t size) noexcept {
    const std::size_t start = m_bytes.size();
    m_bytes.resize(start + size);
    return std::span(m_bytes).subspan(start);
}

// The reading.

Record Recording::Cursor::Next() noexcept {
    const auto type = static_cast<RecordType>(ReadByte(m_bytes, m_offset));
    const std::uint32_t size = ReadLittleEndian<std::uint32_t>(m_bytes, m_offset + 1);
    const auto content = m_bytes.subspan(m_offset + kRecordFrame, size);
    m_offset += kRecordFrame + size;
    return Decode(type, content);
}

Result<Recording> ParseRecording(std::vector<std::byte> bytes) {
    if (bytes.size() > kLargestRecording) {
        return MakeError("the recording has " + std::to_string(bytes.size()) +
                         " bytes, more than the limit of " + std::to_string(kLargestRecording));
    }
    Recording recording;
    const auto firstRecord = ReadHeader(bytes, recording.m_header);
    if (!firstRecord) {
        return tl::unexpected(firstRecord.error());
    }

    SequenceCheck sequence;
    for (std::size_t offset = *firstRecord; offset < bytes.size();) {
        const auto frame = ReadFrame(bytes, offset);
        if (!frame) {
            return tl::unexpected(frame.error());
        }
        if (auto checked = sequence.Check(*frame->shape, Decode(frame->shape->type, frame->content), offset);
            !checked) {
            return tl::unexpected(checked.error());
        }
        offset += kRecordFrame + frame->content.size();
    }
    if (auto finished = sequence.Finish(); !finished) {
        return tl::unexpected(finished.error());
    }

    recording.m_bytes = std::move(bytes);
    recording.m_firstRecord = *firstRecord;
    recording.m_messages = sequence.Messages();
    recording.m_ticks = sequence.Ticks();
    recording.m_checkpoints = sequence.Checkpoints();
    recording.m_end = sequence.End();
    return recording;
}

Result<Recording> ReadRecording(const std::filesystem::path& path) {
    auto bytes = nexum::platform::ReadFileBytes(path, kLargestRecording);
    if (!bytes) {
        return tl::unexpected(bytes.error());
    }
    return ParseRecording(std::move(*bytes));
}

} // namespace mu::recording
