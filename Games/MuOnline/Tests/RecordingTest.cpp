// The .mucap recording, version 1 (CA-03 of slice 2): the bytes of a small recording, fixed by hand from the
// layout of the plan of slice 2 ("O formato .mucap"), the way back, each refusal of the reading with its
// exact reason, and the writer in its reserved room.
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Recording/Recording.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/WorldEvent.hpp"

namespace {

namespace recording = mu::recording;
using Bytes = std::vector<std::byte>;
using mu::session::SessionEpoch;
using recording::Checkpoint;
using recording::EndReason;
using recording::EnteredWorld;
using recording::EpochChanged;
using recording::RecordingEnd;
using recording::RecordingHeader;
using recording::RecordingWriter;
using recording::TickMark;
using recording::WorldMessage;

[[nodiscard]] Bytes Make(std::initializer_list<std::uint8_t> values) {
    Bytes bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

void Append(Bytes& bytes, const Bytes& more) {
    bytes.insert(bytes.end(), more.begin(), more.end());
}

void AppendLittleEndian(Bytes& bytes, std::uint64_t value, std::size_t size) {
    for (std::size_t index = 0; index < size; ++index) {
        bytes.push_back(std::byte{static_cast<std::uint8_t>(value >> (8U * index))});
    }
}

[[nodiscard]] RecordingHeader TestHeader() {
    return RecordingHeader{.nexumVersion = "0.3.0", .dialectVersion = "20404", .server = "OpenMU 0.10.1"};
}

// The header of TestHeader, 38 bytes: the first record starts at byte 38.
[[nodiscard]] Bytes HeaderBytes() {
    return Make({
        0x4E, 0x58, 0x4D, 0x55, 0x43, 0x41, 0x50, 0x1A,                         // NXMUCAP and 1A
        0x01, 0x00,                                                             // format version 1
        0x01, 0x00,                                                             // digest version 1
        0x05, 0x30, 0x2E, 0x33, 0x2E, 0x30,                                     // "0.3.0"
        0x05, 0x32, 0x30, 0x34, 0x30, 0x34,                                     // "20404"
        0x0D, 0x4F, 0x70, 0x65, 0x6E, 0x4D, 0x55, 0x20, 0x30, 0x2E, 0x31, 0x30, // "OpenMU 0.10.1"
        0x2E, 0x31,
    });
}

// The first bytes of the header, cut there.
[[nodiscard]] Bytes HeaderFirst(std::size_t size) {
    Bytes header = HeaderBytes();
    header.resize(size);
    return header;
}

// An object leaves the view (C1 14), 6 bytes.
[[nodiscard]] Bytes Leaving() {
    return Make({0xC1, 0x06, 0x14, 0x01, 0x00, 0x23});
}

// A small recording, byte by byte: the entry in the world at byte 38, a message at 57, a tick at 80, a
// checkpoint at 97, a change of epoch at 130 and the end at 147, 161 bytes in all.
[[nodiscard]] Bytes SmallRecording() {
    Bytes bytes = HeaderBytes();
    Append(bytes, Make({
                      0x01, 0x0E, 0x00, 0x00, 0x00,                   // entry in the world, 14 bytes
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 0 us
                      0x02, 0x00, 0x00, 0x00,                         // epoch 2
                      0x00, 0x02,                                     // hero 0x0200
                      0x02, 0x12, 0x00, 0x00, 0x00,                   // message, 18 bytes
                      0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 1,500 us
                      0x02, 0x00, 0x00, 0x00,                         // epoch 2
                      0xC1, 0x06, 0x14, 0x01, 0x00, 0x23,             // the message
                      0x04, 0x0C, 0x00, 0x00, 0x00,                   // tick, 12 bytes
                      0x40, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 40,000 us
                      0x01, 0x00, 0x00, 0x00,                         // 1 event
                      0x05, 0x1C, 0x00, 0x00, 0x00,                   // checkpoint, 28 bytes
                      0x40, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 40,000 us
                      0x01, 0x00, 0x00, 0x00,                         // 1 message so far
                      0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // digest 0x0123456789ABCDEF
                      0x01, 0x00, 0x00, 0x00,                         // 1 object
                      0x02, 0x00, 0x00, 0x00,                         // 2 items
                      0x03, 0x0C, 0x00, 0x00, 0x00,                   // change of epoch, 12 bytes
                      0x50, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 50,000 us
                      0x03, 0x00, 0x00, 0x00,                         // epoch 3
                      0x06, 0x09, 0x00, 0x00, 0x00,                   // end, 9 bytes
                      0x60, 0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // at 60,000 us
                      0x00,                                           // the client was closed
                  }));
    return bytes;
}

void WriteSmallRecording(RecordingWriter& writer) {
    const Bytes leaving = Leaving();
    CHECK(
        writer.Write(EnteredWorld{.time = 0, .epoch = SessionEpoch{2}, .hero = mu::world::ObjectId{0x0200}}));
    CHECK(writer.Write(WorldMessage{.time = 1'500, .epoch = SessionEpoch{2}, .bytes = leaving}));
    CHECK(writer.Write(TickMark{.time = 40'000, .events = 1}));
    CHECK(writer.Write(Checkpoint{
        .time = 40'000, .messages = 1, .digest = 0x0123'4567'89AB'CDEF, .objects = 1, .items = 2}));
    CHECK(writer.Write(EpochChanged{.time = 50'000, .epoch = SessionEpoch{3}}));
    CHECK(writer.Write(RecordingEnd{.time = 60'000, .reason = EndReason::Closed}));
}

// A recording put together record by record, valid or not, after the header of HeaderBytes. The sizes of
// the content are the right ones unless the test gives another.
class RawRecording {
public:
    RawRecording() : m_bytes(HeaderBytes()) {}

    RawRecording& Record(std::uint8_t type, std::uint64_t contentSize, const Bytes& content) {
        m_bytes.push_back(std::byte{type});
        AppendLittleEndian(m_bytes, contentSize, 4);
        Append(m_bytes, content);
        return *this;
    }

    RawRecording& Entry(std::uint64_t time) {
        return Record(1, 14, Content(time, {0x02, 0x00, 0x00, 0x00, 0x00, 0x02}));
    }
    // A message of 6 bytes: 23 bytes in all.
    RawRecording& Message(std::uint64_t time) {
        Bytes content = Content(time, {0x02, 0x00, 0x00, 0x00});
        Append(content, Leaving());
        return Record(2, 18, content);
    }
    RawRecording& Epoch(std::uint64_t time, std::uint8_t epoch) {
        return Record(3, 12, Content(time, {epoch, 0x00, 0x00, 0x00}));
    }
    RawRecording& Tick(std::uint64_t time, std::uint8_t events = 0) {
        return Record(4, 12, Content(time, {events, 0x00, 0x00, 0x00}));
    }
    RawRecording& Checkpoint(std::uint64_t time, std::uint8_t messages) {
        return Record(5, 28, Content(time, {messages, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                            0x07,     0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
    }
    RawRecording& End(std::uint64_t time, std::uint8_t reason = 0) {
        return Record(6, 9, Content(time, {reason}));
    }

    [[nodiscard]] Bytes Take() { return std::move(m_bytes); }

    // The content of a record: the time, then the rest.
    [[nodiscard]] static Bytes Content(std::uint64_t time, std::initializer_list<std::uint8_t> rest) {
        Bytes content;
        AppendLittleEndian(content, time, 8);
        Append(content, Make(rest));
        return content;
    }

private:
    Bytes m_bytes;
};

// The reason the reading gives.
[[nodiscard]] std::string Refusal(Bytes bytes) {
    const auto read = recording::ParseRecording(std::move(bytes));
    REQUIRE_FALSE(read.has_value());
    return read.error().message;
}

[[nodiscard]] std::vector<recording::Record> RecordsOf(const recording::Recording& read) {
    std::vector<recording::Record> records;
    for (auto cursor = read.Records(); !cursor.AtEnd();) {
        records.push_back(cursor.Next());
    }
    return records;
}

[[nodiscard]] Bytes Copy(std::span<const std::byte> bytes) {
    return {bytes.begin(), bytes.end()};
}

// A folder of its own inside the build folder, where CTest runs the tests, emptied at the start of each test.
[[nodiscard]] std::filesystem::path FreshFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::current_path() / "recording_test_output" / name;
    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    std::filesystem::create_directories(folder);
    return folder;
}

[[nodiscard]] std::string Utf8(const std::filesystem::path& path) {
    std::string text;
    for (const char8_t unit : path.u8string()) {
        text.push_back(static_cast<char>(unit));
    }
    return text;
}

} // namespace

TEST_CASE("[0002-CA-03] a small recording is written byte by byte as the format says") {
    auto writer = RecordingWriter::Create(TestHeader(), 1024);
    REQUIRE_MESSAGE(writer.has_value(), (writer ? "" : writer.error().message));
    CHECK(Copy(writer->Bytes()) == HeaderBytes());
    WriteSmallRecording(*writer);
    CHECK(writer->Ended());
    CHECK_FALSE(writer->Full());
    CHECK(Copy(writer->Bytes()) == SmallRecording());
}

TEST_CASE("[0002-CA-03] a small recording is read back with every field") {
    const auto read = recording::ParseRecording(SmallRecording());
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Header().nexumVersion == "0.3.0");
    CHECK(read->Header().dialectVersion == "20404");
    CHECK(read->Header().server == "OpenMU 0.10.1");
    CHECK(read->Messages() == 1);
    CHECK(read->Ticks() == 1);
    CHECK(read->Checkpoints() == 1);
    CHECK(read->End() == EndReason::Closed);

    const auto records = RecordsOf(*read);
    REQUIRE(records.size() == 6);
    const auto& entry = std::get<EnteredWorld>(records[0]);
    CHECK(entry.time == 0);
    CHECK(entry.epoch == SessionEpoch{2});
    CHECK(entry.hero == mu::world::ObjectId{0x0200});
    const auto& message = std::get<WorldMessage>(records[1]);
    CHECK(message.time == 1'500);
    CHECK(message.epoch == SessionEpoch{2});
    CHECK(Copy(message.bytes) == Leaving());
    const auto& tick = std::get<TickMark>(records[2]);
    CHECK(tick.time == 40'000);
    CHECK(tick.events == 1);
    const auto& checkpoint = std::get<Checkpoint>(records[3]);
    CHECK(checkpoint.time == 40'000);
    CHECK(checkpoint.messages == 1);
    CHECK(checkpoint.digest == 0x0123'4567'89AB'CDEF);
    CHECK(checkpoint.objects == 1);
    CHECK(checkpoint.items == 2);
    const auto& epoch = std::get<EpochChanged>(records[4]);
    CHECK(epoch.time == 50'000);
    CHECK(epoch.epoch == SessionEpoch{3});
    const auto& end = std::get<RecordingEnd>(records[5]);
    CHECK(end.time == 60'000);
    CHECK(end.reason == EndReason::Closed);
}

TEST_CASE("[0002-CA-03] the messages of a recording stay valid when the recording moves") {
    auto read = recording::ParseRecording(SmallRecording());
    REQUIRE(read.has_value());
    auto cursor = read->Records();
    static_cast<void>(cursor.Next());
    const recording::Recording moved = std::move(*read);
    CHECK(Copy(std::get<WorldMessage>(cursor.Next()).bytes) == Leaving());
    CHECK(RecordsOf(moved).size() == 6);
}

TEST_CASE("[0002-CA-03] a session that failed before the world records only the header and the end") {
    auto writer = RecordingWriter::Create(TestHeader(), 64);
    REQUIRE(writer.has_value());
    CHECK(writer->Write(RecordingEnd{.time = 2'000'000, .reason = EndReason::SessionFailed}));
    const auto read = recording::ParseRecording(Copy(writer->Bytes()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Messages() == 0);
    CHECK(read->End() == EndReason::SessionFailed);
    REQUIRE(RecordsOf(*read).size() == 1);
}

TEST_CASE("[0002-CA-03] the reading refuses a wrong mark, an unknown version and a long or cut header") {
    const std::string notMucap = "the recording does not start with the mark of a .mucap";
    CHECK(Refusal({}) == notMucap);
    CHECK(Refusal(Make({0x4E, 0x58, 0x4D})) == notMucap);
    Bytes mark = SmallRecording();
    mark[7] = std::byte{0x0A};
    CHECK(Refusal(mark) == notMucap);

    CHECK(Refusal(HeaderFirst(11)) == "the header is cut");
    Bytes format = SmallRecording();
    format[8] = std::byte{0x02};
    CHECK(Refusal(format) == "the format version 2 is unknown: this Nexum reads the version 1");
    Bytes digest = SmallRecording();
    digest[10] = std::byte{0x00};
    CHECK(Refusal(digest) == "the digest version 0 is unknown: this Nexum reads the version 1");

    Bytes longText = HeaderFirst(12);
    longText.push_back(std::byte{65});
    longText.insert(longText.end(), 65, std::byte{'1'});
    CHECK(Refusal(longText) == "the Nexum version in the header has 65 bytes, more than 64");
    Bytes longServer = HeaderFirst(24);
    longServer.push_back(std::byte{200});
    CHECK(Refusal(longServer) == "the server in the header has 200 bytes, more than 64");

    // Cut in the middle of the server, and right before it.
    CHECK(Refusal(HeaderFirst(30)) == "the header is cut");
    CHECK(Refusal(HeaderFirst(24)) == "the header is cut");
}

TEST_CASE("[0002-CA-03] a text of 64 bytes is the longest the header takes") {
    RecordingHeader header = TestHeader();
    header.server = std::string(64, 's');
    auto writer = RecordingWriter::Create(header, 256);
    REQUIRE_MESSAGE(writer.has_value(), (writer ? "" : writer.error().message));
    CHECK(writer->Write(RecordingEnd{.time = 0, .reason = EndReason::Closed}));
    const auto read = recording::ParseRecording(Copy(writer->Bytes()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Header().server == header.server);
}

TEST_CASE("[0002-CA-03] the texts of the header are UTF-8 without control characters, or the reading refuses "
          "them") {
    // The server comes after the two versions: its size at byte 24, its text from byte 25.
    const auto withServer = [](std::initializer_list<std::uint8_t> text) {
        Bytes bytes = HeaderFirst(24);
        bytes.push_back(std::byte{static_cast<std::uint8_t>(text.size())});
        Append(bytes, Make(text));
        return bytes;
    };
    const std::string control = "the server in the header has a control character at byte ";
    // The escape that starts the sequence clearing a terminal, a byte 0, DEL, and U+009B, a control of C1.
    CHECK(Refusal(withServer({'a', 0x1B, '[', '2', 'J'})) == control + "26");
    CHECK(Refusal(withServer({0x00})) == control + "25");
    CHECK(Refusal(withServer({'a', 'b', 0x7F})) == control + "27");
    CHECK(Refusal(withServer({'a', 0xC2, 0x9B})) == control + "26");
    // A new line in the Nexum version, whose text starts at byte 13.
    Bytes newLine = HeaderFirst(12);
    Append(newLine, Make({0x02, '0', 0x0A}));
    CHECK(Refusal(newLine) == "the Nexum version in the header has a control character at byte 14");

    const std::string notUtf8 = "the server in the header is not UTF-8 at byte ";
    // A byte that only follows another, a byte UTF-8 never has, and the longer form of "/".
    CHECK(Refusal(withServer({0x80})) == notUtf8 + "25");
    CHECK(Refusal(withServer({'a', 0xFF})) == notUtf8 + "26");
    CHECK(Refusal(withServer({'a', 0xC0, 0xAF})) == notUtf8 + "26");
    // A surrogate (U+D800), a character past U+10FFFF, and a second and a third byte that do not follow.
    CHECK(Refusal(withServer({0xED, 0xA0, 0x80})) == notUtf8 + "25");
    CHECK(Refusal(withServer({0xF4, 0x90, 0x80, 0x80})) == notUtf8 + "25");
    CHECK(Refusal(withServer({0xE2, 0x28, 0xA1})) == notUtf8 + "25");
    CHECK(Refusal(withServer({'a', 0xE2, 0x82, 0x28})) == notUtf8 + "26");
    // The "€" cut by the end of the text, though the bytes after it would complete it.
    Bytes cut = withServer({'a', 'b', 0xE2, 0x82});
    cut.push_back(std::byte{0xAC});
    CHECK(Refusal(cut) == notUtf8 + "27");
}

TEST_CASE("[0002-CA-03] the writer refuses a text of the header that the reading would refuse, and takes any "
          "other character") {
    RecordingHeader header = TestHeader();
    header.server = "a\x1b[2J";
    const auto escape = RecordingWriter::Create(header, 1024);
    REQUIRE_FALSE(escape.has_value());
    CHECK(escape.error().message == "the server in the header has a control character at byte 26");
    header = TestHeader();
    header.dialectVersion = "20\xFF"
                            "04";
    const auto notUtf8 = RecordingWriter::Create(header, 1024);
    REQUIRE_FALSE(notUtf8.has_value());
    CHECK(notUtf8.error().message == "the dialect version in the header is not UTF-8 at byte 21");

    // U+00A0, the first after the controls of C1, "ç", the last before the surrogates and the first after
    // them, "€", an emoji and U+10FFFF, the last character.
    header = TestHeader();
    header.server =
        "\xC2\xA0 \xC3\xA7 \xED\x9F\xBF \xEE\x80\x80 \xE2\x82\xAC \xF0\x9F\x98\x80 \xF4\x8F\xBF\xBF";
    auto writer = RecordingWriter::Create(header, 1024);
    REQUIRE_MESSAGE(writer.has_value(), (writer ? "" : writer.error().message));
    CHECK(writer->Write(RecordingEnd{.time = 0, .reason = EndReason::Closed}));
    const auto read = recording::ParseRecording(Copy(writer->Bytes()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Header().server == header.server);
}

TEST_CASE("[0002-CA-03] the reading refuses a record that is cut, of an unknown type or of a wrong size") {
    // Fewer than the 5 bytes of the type and the size.
    Bytes shortFrame = HeaderBytes();
    Append(shortFrame, Make({0x01, 0x0E, 0x00, 0x00}));
    CHECK(Refusal(shortFrame) == "the record at byte 38 is cut");
    // The content ends before its size.
    Bytes shortContent = RawRecording().Entry(0).Take();
    shortContent.resize(shortContent.size() - 4);
    CHECK(Refusal(shortContent) == "the record at byte 38 is cut");

    CHECK(Refusal(RawRecording().Record(7, 12, RawRecording::Content(0, {0, 0, 0, 0})).Take()) ==
          "the record at byte 38 has the unknown type 7");
    CHECK(Refusal(RawRecording().Record(0, 12, RawRecording::Content(0, {0, 0, 0, 0})).Take()) ==
          "the record at byte 38 has the unknown type 0");

    // The entry in the world takes 19 bytes: the next record is at byte 57.
    CHECK(Refusal(RawRecording().Record(1, 15, RawRecording::Content(0, {2, 0, 0, 0, 0, 2, 0})).Take()) ==
          "the entry in the world at byte 38 has 15 bytes of content, not 14");
    CHECK(Refusal(RawRecording().Entry(0).Record(4, 13, RawRecording::Content(0, {0, 0, 0, 0, 0})).Take()) ==
          "the tick at byte 57 has 13 bytes of content, not 12");
    CHECK(Refusal(RawRecording().Entry(0).Record(4, 11, RawRecording::Content(0, {0, 0, 0})).Take()) ==
          "the tick at byte 57 has 11 bytes of content, not 12");
    CHECK(Refusal(RawRecording()
                      .Entry(0)
                      .Record(2, 14, RawRecording::Content(0, {2, 0, 0, 0, 0xC1, 0x02}))
                      .Take()) == "the message at byte 57 has 14 bytes of content, not 15 to 65547");
    // A size above the limit is refused for its size, before the record is found cut.
    CHECK(Refusal(RawRecording().Entry(0).Record(2, 65'548, {}).Take()) ==
          "the message at byte 57 has 65548 bytes of content, not 15 to 65547");
    CHECK(Refusal(RawRecording().Entry(0).Record(4, 0xFFFF'FFFF, {}).Take()) ==
          "the tick at byte 57 has 4294967295 bytes of content, not 12");

    CHECK(Refusal(RawRecording().End(0, 3).Take()) == "the end at byte 38 has the unknown reason 3");
}

TEST_CASE("[0002-CA-03] the reading refuses the time going back and anything in the world before the entry") {
    CHECK(Refusal(RawRecording().Entry(100).Message(99).End(200).Take()) ==
          "the message at byte 57 goes back in time: 99 us after 100 us");
    // The same time is not going back.
    CHECK(recording::ParseRecording(RawRecording().Entry(100).Message(100).Tick(100, 1).End(100).Take())
              .has_value());

    CHECK(Refusal(RawRecording().Message(0).Entry(0).End(0).Take()) ==
          "the message at byte 38 comes before the entry in the world");
    CHECK(Refusal(RawRecording().Epoch(0, 3).Entry(0).End(0).Take()) ==
          "the change of epoch at byte 38 comes before the entry in the world");
    CHECK(Refusal(RawRecording().Tick(0).Entry(0).End(0).Take()) ==
          "the tick at byte 38 comes before the entry in the world");
    CHECK(Refusal(RawRecording().Entry(0).Entry(1).End(2).Take()) ==
          "the entry in the world at byte 57 is the second one");
}

TEST_CASE(
    "[0002-CA-03] the reading refuses a checkpoint that is not right after a tick or that counts wrong") {
    // Entry at 38, message at 57, tick at 80, checkpoint at 97.
    CHECK(Refusal(RawRecording().Entry(0).Message(1).Checkpoint(2, 1).End(3).Take()) ==
          "the checkpoint at byte 80 does not come right after a tick");
    CHECK(Refusal(RawRecording().Checkpoint(0, 0).End(0).Take()) ==
          "the checkpoint at byte 38 does not come right after a tick");
    CHECK(Refusal(RawRecording().Entry(0).Message(1).Tick(2, 1).Checkpoint(2, 2).End(3).Take()) ==
          "the checkpoint at byte 97 counts 2 messages, but the recording has 1 before it");
    CHECK(Refusal(RawRecording().Entry(0).Message(1).Tick(2, 1).Checkpoint(2, 0).End(3).Take()) ==
          "the checkpoint at byte 97 counts 0 messages, but the recording has 1 before it");
    // Two checkpoints after one tick: the second is not right after a tick. Checkpoint at 97, the next at
    // 130.
    CHECK(
        Refusal(
            RawRecording().Entry(0).Message(1).Tick(2, 1).Checkpoint(2, 1).Checkpoint(3, 1).End(3).Take()) ==
        "the checkpoint at byte 130 does not come right after a tick");

    const auto read = recording::ParseRecording(
        RawRecording().Entry(0).Message(1).Tick(2, 1).Checkpoint(2, 1).End(3).Take());
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Checkpoints() == 1);
}

TEST_CASE("[0002-CA-03] the reading refuses anything after the end, and a recording without an end") {
    // Entry at 38, end at 57, the next record at 71.
    CHECK(Refusal(RawRecording().Entry(0).End(1).Tick(2).Take()) ==
          "the tick at byte 71 comes after the end");
    CHECK(Refusal(RawRecording().Entry(0).End(1).End(2).Take()) == "the end at byte 71 comes after the end");
    Bytes extra = RawRecording().Entry(0).End(1).Take();
    extra.push_back(std::byte{0x06});
    CHECK(Refusal(extra) == "the record at byte 71 is cut");

    CHECK(Refusal(RawRecording().Entry(0).Message(1).Take()) == "the recording has no end");
    CHECK(Refusal(HeaderBytes()) == "the recording has no end");
}

TEST_CASE("[0002-CA-03] a recording above 64 MiB is refused for its size, and one of 64 MiB is checked") {
    CHECK(Refusal(Bytes(recording::kLargestRecording + 1)) ==
          "the recording has 67108865 bytes, more than the limit of 67108864");
    // Exactly at the limit, the size passes, and the next check speaks.
    CHECK(Refusal(Bytes(recording::kLargestRecording)) ==
          "the recording does not start with the mark of a .mucap");
}

TEST_CASE(
    "[0002-CA-03] a file of a recording is read whole, and one above 64 MiB is refused before its content") {
    const std::filesystem::path folder = FreshFolder("files");
    const std::filesystem::path small = folder / "small.mucap";
    {
        std::ofstream output(small, std::ios::binary);
        for (const std::byte byte : SmallRecording()) {
            output.put(static_cast<char>(byte));
        }
    }
    const auto read = recording::ReadRecording(small);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(RecordsOf(*read).size() == 6);

    // The size grows without writing the content, so no byte of it exists to be read.
    const std::filesystem::path large = folder / "large.mucap";
    {
        std::ofstream output(large, std::ios::binary);
    }
    std::filesystem::resize_file(large, recording::kLargestRecording + 1);
    const auto refused = recording::ReadRecording(large);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message ==
          "the file " + Utf8(large) + " has 67108865 bytes, more than the limit of 67108864");
}

TEST_CASE("[0002-CA-03] the writer refuses a long text and a room too small for the header and the end") {
    RecordingHeader header = TestHeader();
    header.server = std::string(65, 's');
    const auto longServer = RecordingWriter::Create(header, 1024);
    REQUIRE_FALSE(longServer.has_value());
    CHECK(longServer.error().message == "the server in the header has 65 bytes, more than 64");
    header = TestHeader();
    header.nexumVersion = std::string(100, 'v');
    const auto longVersion = RecordingWriter::Create(header, 1024);
    REQUIRE_FALSE(longVersion.has_value());
    CHECK(longVersion.error().message == "the Nexum version in the header has 100 bytes, more than 64");

    // The header takes 38 bytes and the end 14.
    const auto tooSmall = RecordingWriter::Create(TestHeader(), 51);
    REQUIRE_FALSE(tooSmall.has_value());
    CHECK(tooSmall.error().message ==
          "the room of 51 bytes is too small for the header and the end, which take 52");
    auto justTheEnd = RecordingWriter::Create(TestHeader(), 52);
    REQUIRE(justTheEnd.has_value());
    CHECK_FALSE(justTheEnd->Write(EnteredWorld{.time = 0, .epoch = SessionEpoch{2}, .hero = {}}));
    CHECK(justTheEnd->Full());
    CHECK(justTheEnd->Write(RecordingEnd{.time = 0, .reason = EndReason::Full}));
    CHECK(justTheEnd->Bytes().size() == 52);
}

TEST_CASE("[0002-CA-03] a full writer keeps the room for the end, takes nothing else, and stays valid") {
    // The header (38), the entry (19), one message (23) and the end (14).
    auto writer = RecordingWriter::Create(TestHeader(), 94);
    REQUIRE(writer.has_value());
    const Bytes leaving = Leaving();
    CHECK(writer->Write(
        EnteredWorld{.time = 0, .epoch = SessionEpoch{2}, .hero = mu::world::ObjectId{0x0200}}));
    CHECK(writer->Write(WorldMessage{.time = 1, .epoch = SessionEpoch{2}, .bytes = leaving}));
    CHECK_FALSE(writer->Full());
    CHECK_FALSE(writer->Write(WorldMessage{.time = 2, .epoch = SessionEpoch{2}, .bytes = leaving}));
    CHECK(writer->Full());
    // Full, it takes nothing but the end, even a record that would fit.
    CHECK_FALSE(writer->Write(TickMark{.time = 2, .events = 0}));
    CHECK(writer->Bytes().size() == 80);
    CHECK(writer->Write(RecordingEnd{.time = 3, .reason = EndReason::Full}));
    CHECK(writer->Bytes().size() == 94);
    // After the end, nothing.
    CHECK_FALSE(writer->Write(RecordingEnd{.time = 4, .reason = EndReason::Closed}));
    CHECK(writer->Ended());
    CHECK(writer->Bytes().size() == 94);

    const auto read = recording::ParseRecording(Copy(writer->Bytes()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Messages() == 1);
    CHECK(read->End() == EndReason::Full);
}

TEST_CASE("[0002-CA-03] the writer takes messages from 3 to 65,535 bytes, and no other") {
    auto writer = RecordingWriter::Create(TestHeader(), 128 * 1024);
    REQUIRE(writer.has_value());
    CHECK(writer->Write(
        EnteredWorld{.time = 0, .epoch = SessionEpoch{2}, .hero = mu::world::ObjectId{0x0200}}));
    const Bytes two(2, std::byte{0xC1});
    const Bytes three = Make({0xC1, 0x03, 0x00});
    const Bytes largest(recording::kLongestMessage, std::byte{0xC2});
    const Bytes tooLarge(recording::kLongestMessage + 1, std::byte{0xC2});
    CHECK_FALSE(writer->Write(WorldMessage{.time = 1, .epoch = SessionEpoch{2}, .bytes = two}));
    CHECK_FALSE(writer->Write(WorldMessage{.time = 1, .epoch = SessionEpoch{2}, .bytes = tooLarge}));
    CHECK_FALSE(writer->Full());
    CHECK(writer->Write(WorldMessage{.time = 1, .epoch = SessionEpoch{2}, .bytes = three}));
    CHECK(writer->Write(WorldMessage{.time = 2, .epoch = SessionEpoch{2}, .bytes = largest}));
    CHECK(writer->Write(RecordingEnd{.time = 3, .reason = EndReason::Closed}));

    const auto read = recording::ParseRecording(Copy(writer->Bytes()));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    const auto records = RecordsOf(*read);
    REQUIRE(records.size() == 4);
    CHECK(Copy(std::get<WorldMessage>(records[1]).bytes) == three);
    CHECK(Copy(std::get<WorldMessage>(records[2]).bytes) == largest);
}
