#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include "AllocationCounter.hpp"
#include "Nexum/Platform/FileBytes.hpp"

using frame_allocations::AllocationCounter;
using frame_allocations::AllocationTally;
using nexum::platform::ReadFileBytes;

namespace {

// 1 MiB: far more than the memory the reading asks for besides the content, which is the path and the error
// message, a few hundred bytes.
constexpr std::size_t kFileSize = std::size_t{1} << 20U;
constexpr std::uint64_t kMostBytesWithoutTheContent = 64U * 1024U;

std::filesystem::path MakeFileOfOneMebibyte() {
    const std::filesystem::path folder =
        std::filesystem::current_path() / "file_bytes_allocation_test_output";
    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    std::filesystem::create_directories(folder);
    const std::filesystem::path file = folder / "big.bin";
    std::ofstream output(file, std::ios::binary);
    const std::string block(1024, 'x');
    for (std::size_t written = 0; written < kFileSize; written += block.size()) {
        output.write(block.data(), static_cast<std::streamsize>(block.size()));
    }
    return file;
}

} // namespace

// The counter sees the memory of the content only when it is asked for, so a limit checked after the reading
// would show here as a megabyte counted. SDL asks for its own memory straight from the C library, outside the
// counter (tests/architecture/frame_allocations).
TEST_CASE("[0002-CA-03] a file above the limit is refused before any memory is asked for its content") {
    const std::filesystem::path file = MakeFileOfOneMebibyte();
    REQUIRE(std::filesystem::file_size(file) == kFileSize);

    AllocationTally tally;
    bool refused = false;
    {
        const AllocationCounter counter;
        refused = !ReadFileBytes(file, kFileSize - 1).has_value();
        tally = counter.Tally();
    }
    CHECK(refused);
    CHECK_MESSAGE(tally.bytes < kMostBytesWithoutTheContent, tally.bytes, " bytes asked in ", tally.requests,
                  " requests");
}

// The control case: within the limit, the same file is read, and the counter sees the megabyte of its
// content.
TEST_CASE("[0002-CA-03] the counter sees the memory of the content of a file that is read") {
    const std::filesystem::path file = MakeFileOfOneMebibyte();

    AllocationTally tally;
    std::size_t size = 0;
    {
        const AllocationCounter counter;
        const auto read = ReadFileBytes(file, kFileSize);
        size = read ? read->size() : 0;
        tally = counter.Tally();
    }
    CHECK(size == kFileSize);
    CHECK(tally.bytes >= kFileSize);
}
