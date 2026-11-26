#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/Png.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using nexum::platform::DecodePng;
using nexum::platform::kMaxPngSide;
using nexum::platform::LoadPng;
using nexum::platform::ReadPngSize;
using nexum::platform::SavePng;

namespace {

// A folder of its own inside the build folder, where CTest runs the tests, emptied at the start of each test.
std::filesystem::path FreshFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::current_path() / "png_test_output" / name;
    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    std::filesystem::create_directories(folder);
    return folder;
}

// Every pixel different, and every channel used, the alpha included, so a swapped channel or a lost row
// shows.
Image MakeTestImage() {
    Image image(3, 2);
    image.At(0, 0) = Rgba8{.r = 255, .g = 0, .b = 0, .a = 255};
    image.At(1, 0) = Rgba8{.r = 0, .g = 255, .b = 0, .a = 128};
    image.At(2, 0) = Rgba8{.r = 0, .g = 0, .b = 255, .a = 1};
    image.At(0, 1) = Rgba8{.r = 10, .g = 20, .b = 30, .a = 40};
    image.At(1, 1) = Rgba8{.r = 250, .g = 240, .b = 230, .a = 0};
    image.At(2, 1) = Rgba8{.r = 1, .g = 2, .b = 3, .a = 254};
    return image;
}

// A PNG file of 2x1 pixels made outside the Nexum, with the zlib of Python: an opaque red pixel and a blue
// one with half of the alpha. The round trip below would not see the red and blue channels swapped in both
// directions; this file would.
constexpr std::array<unsigned char, 71> kKnownPng = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44,
    0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0xF4,
    0x22, 0x7F, 0x8A, 0x00, 0x00, 0x00, 0x0E, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xF8,
    0xCF, 0xC0, 0x00, 0x42, 0x0D, 0x00, 0x0F, 0x7A, 0x03, 0x7E, 0x6A, 0x81, 0x31, 0xE1, 0x00,
    0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
};

} // namespace

TEST_CASE("[0000-CA-07] a PNG file made elsewhere is read with its channels in order") {
    const std::filesystem::path file = FreshFolder("known") / "known.png";
    {
        std::ofstream output(file, std::ios::binary);
        for (const unsigned char byte : kKnownPng) {
            output.put(static_cast<char>(byte));
        }
    }

    const auto read = LoadPng(file);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    REQUIRE(read->Width() == 2);
    REQUIRE(read->Height() == 1);
    CHECK(read->At(0, 0) == Rgba8{.r = 255, .g = 0, .b = 0, .a = 255});
    CHECK(read->At(1, 0) == Rgba8{.r = 0, .g = 0, .b = 255, .a = 128});
}

TEST_CASE("[0000-CA-07] a PNG file gives back the exact pixels that were written") {
    const std::filesystem::path file = FreshFolder("round_trip") / "image.png";
    const Image written = MakeTestImage();

    const auto saved = SavePng(written, file);
    REQUIRE_MESSAGE(saved.has_value(), (saved ? "" : saved.error().message));
    const auto read = LoadPng(file);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));

    CHECK(read->Width() == 3);
    CHECK(read->Height() == 2);
    CHECK(*read == written);
}

TEST_CASE("[0000-CA-07] writing a PNG file replaces the file that was there") {
    const std::filesystem::path file = FreshFolder("replace") / "image.png";
    REQUIRE(SavePng(Image(8, 8, Rgba8{.r = 9, .g = 9, .b = 9, .a = 9}), file).has_value());
    const Image second = MakeTestImage();
    REQUIRE(SavePng(second, file).has_value());

    const auto read = LoadPng(file);
    REQUIRE(read.has_value());
    CHECK(*read == second);
}

TEST_CASE("reading a PNG file that does not exist gives an error") {
    const std::filesystem::path file = FreshFolder("missing") / "nothing.png";
    const auto read = LoadPng(file);
    REQUIRE_FALSE(read.has_value());
    MESSAGE(read.error().message);
    CHECK(read.error().message.starts_with("could not read the PNG file "));
}

TEST_CASE("reading a file that is not a PNG gives an error") {
    const std::filesystem::path file = FreshFolder("not_png") / "text.png";
    std::ofstream(file) << "this is not a PNG file";
    const auto read = LoadPng(file);
    REQUIRE_FALSE(read.has_value());
    MESSAGE(read.error().message);
    CHECK(read.error().message.starts_with("could not read the PNG file "));
}

TEST_CASE("writing a PNG file into a folder that does not exist gives an error") {
    const std::filesystem::path file = FreshFolder("no_folder") / "missing" / "image.png";
    const auto saved = SavePng(MakeTestImage(), file);
    REQUIRE_FALSE(saved.has_value());
    MESSAGE(saved.error().message);
    CHECK(saved.error().message.starts_with("could not write the PNG file "));
}

TEST_CASE("an empty image or one above the size limit is not written") {
    const std::filesystem::path folder = FreshFolder("limits");
    for (const auto& [width, height] : {std::pair<std::uint32_t, std::uint32_t>{0, 0},
                                        std::pair<std::uint32_t, std::uint32_t>{kMaxPngSide + 1, 1}}) {
        CAPTURE(width);
        CAPTURE(height);
        const auto saved = SavePng(Image(width, height), folder / "image.png");
        REQUIRE_FALSE(saved.has_value());
        CHECK(saved.error().message.find("must be between 1 and 16384 pixels on each side") !=
              std::string::npos);
    }
    CHECK_FALSE(std::filesystem::exists(folder / "image.png"));
}

TEST_CASE("[0001-CA-10] a small PNG file that declares a huge image is refused by its header") {
    // The signature and the IHDR chunk of 60000x60000 pixels, then nothing: SDL would ask for about 14 GB
    // before seeing that the rest is missing (docs/TECH_DEBT.md, TD-0020).
    std::array<unsigned char, 33> huge{};
    std::copy_n(kKnownPng.begin(), huge.size(), huge.begin());
    for (const std::size_t offset : {std::size_t{16}, std::size_t{20}}) {
        huge[offset + 2] = 0xEA; // 60000 = 0x0000EA60, big-endian
        huge[offset + 3] = 0x60;
    }
    const std::filesystem::path file = FreshFolder("huge") / "huge.png";
    {
        std::ofstream output(file, std::ios::binary);
        for (const unsigned char byte : huge) {
            output.put(static_cast<char>(byte));
        }
    }
    const auto read = LoadPng(file);
    REQUIRE_FALSE(read.has_value());
    MESSAGE(read.error().message);
    CHECK(read.error().message.find("is 60000x60000 pixels; an image must be between 1 and 16384") !=
          std::string::npos);

    const auto decoded = DecodePng(std::as_bytes(std::span(huge)));
    REQUIRE_FALSE(decoded.has_value());
    CHECK(decoded.error().message ==
          "the PNG file in memory is 60000x60000 pixels; an image must be between 1 "
          "and 16384 pixels on each side");
}

TEST_CASE("[0001-CA-10] the size of a PNG comes from its first 24 bytes") {
    const auto bytes = std::as_bytes(std::span(kKnownPng));
    const auto size = ReadPngSize(bytes);
    REQUIRE(size.has_value());
    CHECK(size->width == 2);
    CHECK(size->height == 1);

    CHECK(ReadPngSize(bytes.first(23)).error().message ==
          "it has 23 bytes, fewer than the 24 of a PNG header");
    auto wrong = kKnownPng;
    wrong[1] = 'X';
    CHECK(ReadPngSize(std::as_bytes(std::span(wrong))).error().message ==
          "it does not start with the PNG signature");
    wrong = kKnownPng;
    wrong[12] = 'X';
    CHECK(ReadPngSize(std::as_bytes(std::span(wrong))).error().message ==
          "its first chunk is not the 13 bytes of IHDR");
}

TEST_CASE("[0001-CA-10] a PNG in memory is read like a file") {
    const auto read = DecodePng(std::as_bytes(std::span(kKnownPng)));
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->Width() == 2);
    CHECK(read->Height() == 1);
    CHECK(read->At(0, 0) == Rgba8{.r = 255, .g = 0, .b = 0, .a = 255});
}
