// The code that packetgen generates from Fixture.xml, a file written for these tests: every kind of field
// lands on the bytes the XML declares, with the byte order and the bit position of each field (CA-03).
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <doctest/doctest.h>

#include "PacketGen/Fixture/EveryField.hpp"
#include "PacketGen/Fixture/Fixed.hpp"
#include "PacketGen/Fixture/PointList.hpp"

namespace {

using packetgen::fixture::EveryField;
using packetgen::fixture::Fixed;
using packetgen::fixture::PointList;

template <std::size_t Size>
[[nodiscard]] std::array<std::byte, Size> Bytes(const std::array<std::uint8_t, Size>& values) {
    std::array<std::byte, Size> bytes{};
    for (std::size_t index = 0; index < Size; ++index) {
        bytes[index] = std::byte{values[index]};
    }
    return bytes;
}

} // namespace

TEST_CASE("[0001-CA-03] a generated writer puts every kind of field on the bytes the XML declares") {
    std::array<std::byte, 31> buffer{};
    const auto writer = EveryField::Writer::Create(buffer);
    REQUIRE(writer.has_value());
    CHECK(buffer[19] == std::byte{2}); // the default value Color.Green, written by Create
    writer->SetHigh(0x0A);
    writer->SetLow(0x05);
    writer->SetLittle(0x1234);
    writer->SetBig(0x1234);
    writer->SetBigWord(0x01020304);
    writer->SetFlag(true);
    REQUIRE(writer->SetLabel("abc").has_value());
    writer->SetPaint(EveryField::Color::Red);
    writer->SetLittleLong(0x0807060504030201);
    const std::array<std::byte, 3> rest{std::byte{0xEE}, std::byte{0xFF}, std::byte{0x11}};
    REQUIRE(writer->SetRest(rest).has_value());

    const auto expected =
        Bytes<31>({0xC1, 31, 0xA1, 0x02, 0xA5, 0x34, 0x12, 0x12, 0x34, 0x01, 0x02, 0x03, 0x04, 0x08, 'a', 'b',
                   'c',  0,  0,    1,    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xEE, 0xFF, 0x11});
    CHECK(buffer == expected);
}

TEST_CASE("[0001-CA-03] a generated view reads every kind of field from the bytes the XML declares") {
    const auto bytes =
        Bytes<30>({0xC3, 30,  0xA1, 0x02, 0x3C, 0xCD, 0xAB, 0xAB, 0xCD, 0xFF, 0x00, 0x00, 0x01, 0xF7, 'h',
                   'e',  'l', 'l',  'o',  2,    0xFF, 0,    0,    0,    0,    0,    0,    0x80, 0x42, 0x43});
    const auto view = EveryField::View::Read(bytes);
    REQUIRE(view.has_value());
    CHECK(view->High() == 0x03);
    CHECK(view->Low() == 0x0C);
    CHECK(view->Little() == 0xABCD);
    CHECK(view->Big() == 0xABCD);
    CHECK(view->BigWord() == 0xFF000001);
    CHECK_FALSE(view->Flag()); // 0xF7 has every bit but bit 3
    CHECK(view->Label().Equals("hello"));
    CHECK(view->Paint() == EveryField::Color::Green);
    CHECK(view->LittleLong() == 0x80000000000000FF);
    CHECK(view->Rest().size() == 2);
}

TEST_CASE(
    "[0001-CA-03] a generated view refuses a message that is not its packet, without reading past the end") {
    std::array<std::byte, 28> bytes{};
    REQUIRE(EveryField::Writer::Create(bytes).has_value());
    CHECK(EveryField::View::Read(bytes).has_value());

    // One byte shorter than the last fixed field, with the length corrected: refused, with the reason.
    std::vector<std::byte> shorter(bytes.begin(), bytes.end() - 1);
    shorter[1] = std::byte{27};
    const auto refused = EveryField::View::Read(shorter);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the EveryField needs at least 28 bytes, but it has 27");

    auto wrongLength = bytes;
    wrongLength[1] = std::byte{29};
    CHECK_FALSE(EveryField::View::Read(wrongLength).has_value());
    auto wrongCode = bytes;
    wrongCode[2] = std::byte{0xA2};
    CHECK(EveryField::View::Read(wrongCode).error().message ==
          "the message is not a EveryField: code A2 instead of A1");
    auto wrongSubCode = bytes;
    wrongSubCode[3] = std::byte{0x03};
    CHECK_FALSE(EveryField::View::Read(wrongSubCode).has_value());
    auto wrongHeader = bytes;
    wrongHeader[0] = std::byte{0xC2};
    CHECK_FALSE(EveryField::View::Read(wrongHeader).has_value());
    CHECK_FALSE(EveryField::View::Read(std::span<const std::byte>(bytes).first(2)).has_value());
}

TEST_CASE("[0001-CA-03] a generated writer refuses a text or a buffer that does not fit") {
    std::array<std::byte, 28> buffer{};
    const auto writer = EveryField::Writer::Create(buffer);
    REQUIRE(writer.has_value());
    CHECK_FALSE(writer->SetLabel("sixsix").has_value());
    CHECK(writer->SetLabel("five5").has_value());

    std::array<std::byte, 27> small{};
    CHECK(EveryField::Writer::Create(small).error().message ==
          "a EveryField has from 28 to 255 bytes, not 27");
    std::vector<std::byte> large(256);
    CHECK_FALSE(EveryField::Writer::Create(large).has_value());
}

TEST_CASE("[0001-CA-03] a generated list counts by its count field and refuses an item past the end") {
    // C2 with a big-endian length of 11, the code B0, the count 2 and two points of 3 bytes: (1, 2) and (3,
    // 4).
    const auto bytes = Bytes<11>({0xC2, 0x00, 11, 0xB0, 2, 1, 2, 0, 3, 4, 0});
    const auto view = PointList::View::Read(bytes);
    REQUIRE(view.has_value());
    CHECK(view->PointsCount() == 2);
    const auto first = view->Points(0);
    REQUIRE(first.has_value());
    CHECK(first->X() == 1);
    CHECK(first->Y() == 2);
    const auto second = view->Points(1);
    REQUIRE(second.has_value());
    CHECK(second->X() == 3);
    CHECK(second->Y() == 4);
    CHECK_FALSE(view->Points(2).has_value());

    // The same packet cut in the middle of the second point: the count still says 2, and the second point is
    // refused.
    const auto cut = Bytes<10>({0xC2, 0x00, 10, 0xB0, 2, 1, 2, 0, 3, 4});
    const auto cutView = PointList::View::Read(cut);
    REQUIRE(cutView.has_value());
    CHECK(cutView->PointsCount() == 2);
    CHECK(cutView->Points(0).has_value());
    CHECK(cutView->Points(1).error().message == "the Point needs 3 bytes, but only 2 are left");

    std::vector<std::byte> buffer(8);
    const auto writer = PointList::Writer::Create(buffer);
    REQUIRE(writer.has_value());
    writer->SetPointCount(1);
    const auto point = writer->Points(0);
    REQUIRE(point.has_value());
    point->SetX(7);
    point->SetY(9);
    const std::vector<std::byte> expected{std::byte{0xC2}, std::byte{0x00}, std::byte{8}, std::byte{0xB0},
                                          std::byte{1},    std::byte{7},    std::byte{9}, std::byte{0}};
    CHECK(buffer == expected);
}

TEST_CASE("[0001-CA-03] a generated packet with a declared length keeps it, and its writer sets the default "
          "value") {
    static_assert(Fixed::kDeclaredLength == 6);
    static_assert(Fixed::kShape.minimumLength == 4);
    std::array<std::byte, Fixed::kDeclaredLength> buffer{};
    REQUIRE(Fixed::Writer::Create(buffer).has_value());
    CHECK(buffer == Bytes<6>({0xC3, 6, 0x0C, 0x28, 0, 0}));
}
