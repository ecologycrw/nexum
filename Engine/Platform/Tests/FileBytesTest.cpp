#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include "Nexum/Platform/FileBytes.hpp"

using nexum::platform::ReadFileBytes;
using nexum::platform::WriteFileBytes;

namespace {

// A folder of its own inside the build folder, where CTest runs the tests, emptied at the start of each test.
std::filesystem::path FreshFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::current_path() / "file_bytes_test_output" / name;
    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    std::filesystem::create_directories(folder);
    return folder;
}

// Every value of a byte, in order, so a byte changed on the way, such as a line end turned into two bytes or
// a zero that ends a text, shows.
std::array<std::byte, 256> EveryByte() {
    std::array<std::byte, 256> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes.at(index) = static_cast<std::byte>(index);
    }
    return bytes;
}

// Writes the file with the C++ library, outside the code under test.
void WriteWithTheLibrary(const std::filesystem::path& file, std::size_t size) {
    std::ofstream output(file, std::ios::binary);
    for (std::size_t index = 0; index < size; ++index) {
        output.put(static_cast<char>('a' + (index % 26)));
    }
}

std::string Utf8(const std::filesystem::path& path) {
    std::string text;
    for (const char8_t unit : path.u8string()) {
        text.push_back(static_cast<char>(unit));
    }
    return text;
}

} // namespace

TEST_CASE("[0002-CA-03] a file is read whole, with every byte as it is on the disk") {
    const std::filesystem::path file = FreshFolder("every_byte") / "bytes.bin";
    const std::array<std::byte, 256> written = EveryByte();
    {
        std::ofstream output(file, std::ios::binary);
        for (const std::byte byte : written) {
            output.put(static_cast<char>(byte));
        }
    }

    const auto read = ReadFileBytes(file, written.size());
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(std::ranges::equal(*read, written));
}

TEST_CASE("[0002-CA-04] written bytes are read back exactly") {
    const std::filesystem::path file = FreshFolder("round_trip") / "bytes.bin";
    const std::array<std::byte, 256> written = EveryByte();

    const auto saved = WriteFileBytes(file, written);
    REQUIRE_MESSAGE(saved.has_value(), (saved ? "" : saved.error().message));
    CHECK(std::filesystem::file_size(file) == written.size());
    const auto read = ReadFileBytes(file, written.size());
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(std::ranges::equal(*read, written));
}

TEST_CASE("[0002-CA-03] a file larger than the limit is refused, and one of exactly the limit is read") {
    const std::filesystem::path folder = FreshFolder("limit");
    WriteWithTheLibrary(folder / "seventeen.bin", 17);
    WriteWithTheLibrary(folder / "sixteen.bin", 16);

    const auto refused = ReadFileBytes(folder / "seventeen.bin", 16);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message ==
          "the file " + Utf8(folder / "seventeen.bin") + " has 17 bytes, more than the limit of 16");

    const auto read = ReadFileBytes(folder / "sixteen.bin", 16);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->size() == 16);
}

TEST_CASE("an empty file is read as no bytes, even with a limit of zero") {
    const std::filesystem::path file = FreshFolder("empty") / "empty.bin";
    REQUIRE(WriteFileBytes(file, std::span<const std::byte>{}).has_value());
    CHECK(std::filesystem::file_size(file) == 0);

    const auto read = ReadFileBytes(file, 0);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(read->empty());
}

TEST_CASE("reading a file that does not exist gives an error with its path") {
    const std::filesystem::path file = FreshFolder("missing") / "nothing.bin";
    const auto read = ReadFileBytes(file, 1024);
    REQUIRE_FALSE(read.has_value());
    MESSAGE(read.error().message);
    CHECK(read.error().message.starts_with("could not read the file " + Utf8(file) + ": "));
}

TEST_CASE("[0002-CA-04] writing a file replaces the whole of what was there") {
    const std::filesystem::path file = FreshFolder("replace") / "bytes.bin";
    REQUIRE(WriteFileBytes(file, EveryByte()).has_value());
    const std::array<std::byte, 3> second{std::byte{7}, std::byte{8}, std::byte{9}};
    REQUIRE(WriteFileBytes(file, second).has_value());

    const auto read = ReadFileBytes(file, 1024);
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(std::ranges::equal(*read, second));
}

TEST_CASE("[0002-CA-04] writing into a folder that does not exist gives an error with its path") {
    const std::filesystem::path file = FreshFolder("no_folder") / "missing" / "bytes.bin";
    const auto saved = WriteFileBytes(file, EveryByte());
    REQUIRE_FALSE(saved.has_value());
    MESSAGE(saved.error().message);
    CHECK(saved.error().message.starts_with("could not write the file " + Utf8(file) + ": "));
    CHECK_FALSE(std::filesystem::exists(file));
}

TEST_CASE("a file whose name has letters outside ASCII is written and read") {
    const std::filesystem::path file = FreshFolder("utf8") / std::filesystem::path(u8"gravação-ñ-日本.bin");
    const std::array<std::byte, 2> written{std::byte{1}, std::byte{2}};

    const auto saved = WriteFileBytes(file, written);
    REQUIRE_MESSAGE(saved.has_value(), (saved ? "" : saved.error().message));
    CHECK(std::filesystem::exists(file));
    const auto read = ReadFileBytes(file, written.size());
    REQUIRE_MESSAGE(read.has_value(), (read ? "" : read.error().message));
    CHECK(std::ranges::equal(*read, written));
}
