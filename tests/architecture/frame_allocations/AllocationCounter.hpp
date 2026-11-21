#pragma once

#include <cstdint>
#include <string>

namespace frame_allocations {

// Whether the counter also records where its first requests came from. Only Linux and macOS give the call
// stack of a thread outside the platform module of the engine.
#if defined(__linux__) || defined(__APPLE__)
inline constexpr bool kRecordsCallStacks = true;
#else
inline constexpr bool kRecordsCallStacks = false;
#endif

// The requests for memory counted on one thread.
struct AllocationTally {
    std::uint64_t requests = 0;
    std::uint64_t bytes = 0;
};

// While it exists, counts the requests for memory that its thread makes through the global allocation
// functions of C++ (new and new[], with their aligned and nothrow forms), which the test program that links
// the counter replaces (CA-09 of specs/0000-fundacao/spec.md). The Nexum asks for memory only through them:
// the cpp-rules check refuses malloc in its code. Not counted are the requests of other threads, such as the
// inner ones of a graphics driver, and the memory that SDL and the drivers ask straight from the C library.
// One counter per thread at a time.
class AllocationCounter {
public:
    // Starts counting from zero.
    AllocationCounter() noexcept;
    ~AllocationCounter();

    AllocationCounter(const AllocationCounter&) = delete;
    AllocationCounter& operator=(const AllocationCounter&) = delete;
    AllocationCounter(AllocationCounter&&) = delete;
    AllocationCounter& operator=(AllocationCounter&&) = delete;

    // What was counted since the counter started.
    [[nodiscard]] AllocationTally Tally() const noexcept;
};

// Where the first 8 requests counted by the last counter of this thread came from: for each one, its size and
// the functions of its call stack, one per line, from the allocation function outwards. Empty when nothing
// was counted, or where kRecordsCallStacks is false. Making the text asks for memory, so call it after the
// counter is gone.
[[nodiscard]] std::string DescribeCountedRequests();

} // namespace frame_allocations
