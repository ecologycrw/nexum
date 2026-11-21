// Replaces the global allocation functions of C++ for the test program that links the counter, to count the
// requests for memory (CA-09 of specs/0000-fundacao/spec.md). The cpp-rules check allows this only in this
// folder.

#include "AllocationCounter.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string>

#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#endif

namespace {

// How many requests keep their call stack, and how many functions of each stack are kept.
constexpr std::size_t kRecordedRequests = 8;
constexpr int kStackDepth = 32;

// One counted request and where it came from. The stack lives in a fixed array, so recording it asks for no
// memory.
struct RecordedRequest {
    std::size_t size = 0;
    int depth = 0;
    std::array<void*, kStackDepth> frames{};
};

// What the calling thread counts. It is initialized as a constant, so an allocation function can read it even
// while a new thread is still starting.
struct ThreadCount {
    bool counting = false;
    // True while a call stack is being taken: a request made by that is neither counted nor recorded.
    bool capturing = false;
    frame_allocations::AllocationTally tally;
    std::array<RecordedRequest, kRecordedRequests> recorded{};
};

thread_local constinit ThreadCount tCount;

void RecordStack(ThreadCount& count, RecordedRequest& request) noexcept {
#if defined(__linux__) || defined(__APPLE__)
    count.capturing = true;
    request.depth = backtrace(request.frames.data(), kStackDepth);
    count.capturing = false;
#else
    static_cast<void>(count);
    static_cast<void>(request);
#endif
}

// The first call that takes a call stack may load a library, which asks for memory: the counter makes it
// before it starts counting.
void PrepareCallStacks() noexcept {
#if defined(__linux__) || defined(__APPLE__)
    std::array<void*, 1> frame{};
    static_cast<void>(backtrace(frame.data(), 1));
#endif
}

void Record(std::size_t size) noexcept {
    ThreadCount& count = tCount;
    if (!count.counting || count.capturing) {
        return;
    }
    const std::uint64_t index = count.tally.requests;
    ++count.tally.requests;
    count.tally.bytes += size;
    if (index < kRecordedRequests) {
        RecordedRequest& request = count.recorded[index];
        request.size = size;
        RecordStack(count, request);
    }
}

#if defined(__linux__) || defined(__APPLE__)
// The size and the call stack of one recorded request, as text.
std::string DescribeRequest(std::size_t number, const RecordedRequest& request) {
    std::string text =
        "request " + std::to_string(number) + " (" + std::to_string(request.size) + " bytes):\n";
    // backtrace_symbols gives all the names in one block from malloc, which the caller frees.
    char** const names = backtrace_symbols(request.frames.data(), request.depth);
    if (names == nullptr) {
        return text + "  (the names of the functions could not be read)\n";
    }
    for (int frame = 0; frame < request.depth; ++frame) {
        text += "  ";
        text += names[frame];
        text += "\n";
    }
    std::free(static_cast<void*>(names));
    return text;
}
#endif

void* Allocate(std::size_t size) noexcept {
    Record(size);
    // operator new gives a distinct address even for zero bytes; malloc might give back nothing.
    return std::malloc(std::max<std::size_t>(size, 1));
}

void* AllocateAligned(std::size_t size, std::align_val_t alignment) noexcept {
    Record(size);
    // The aligned allocation of the C library wants an alignment of at least the size of a pointer.
    const std::size_t align = std::max(static_cast<std::size_t>(alignment), sizeof(void*));
    const std::size_t bytes = std::max<std::size_t>(size, 1);
#if defined(_MSC_VER)
    return _aligned_malloc(bytes, align);
#else
    void* pointer = nullptr;
    return posix_memalign(&pointer, align, bytes) == 0 ? pointer : nullptr;
#endif
}

void Release(void* pointer) noexcept {
    std::free(pointer);
}

void ReleaseAligned(void* pointer) noexcept {
#if defined(_MSC_VER)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

void* AllocateOrThrow(std::size_t size) {
    void* const pointer = Allocate(size);
    if (pointer == nullptr) {
        throw std::bad_alloc();
    }
    return pointer;
}

void* AllocateAlignedOrThrow(std::size_t size, std::align_val_t alignment) {
    void* const pointer = AllocateAligned(size, alignment);
    if (pointer == nullptr) {
        throw std::bad_alloc();
    }
    return pointer;
}

} // namespace

namespace frame_allocations {

AllocationCounter::AllocationCounter() noexcept {
    PrepareCallStacks();
    tCount.tally = AllocationTally{};
    tCount.recorded = {};
    tCount.counting = true;
}

AllocationCounter::~AllocationCounter() {
    tCount.counting = false;
}

AllocationTally AllocationCounter::Tally() const noexcept {
    return tCount.tally;
}

std::string DescribeCountedRequests() {
    std::string text;
#if defined(__linux__) || defined(__APPLE__)
    const ThreadCount& count = tCount;
    const auto recorded =
        static_cast<std::size_t>(std::min<std::uint64_t>(count.tally.requests, kRecordedRequests));
    for (std::size_t index = 0; index < recorded; ++index) {
        text += DescribeRequest(index + 1, count.recorded[index]);
    }
#endif
    return text;
}

} // namespace frame_allocations

// The replaced allocation functions: every form the standard lets a program replace.

void* operator new(std::size_t size) {
    return AllocateOrThrow(size);
}

void* operator new[](std::size_t size) {
    return AllocateOrThrow(size);
}

void* operator new(std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return Allocate(size);
}

void* operator new[](std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return Allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return AllocateAlignedOrThrow(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return AllocateAlignedOrThrow(size, alignment);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t& /*tag*/) noexcept {
    return AllocateAligned(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t& /*tag*/) noexcept {
    return AllocateAligned(size, alignment);
}

void operator delete(void* pointer) noexcept {
    Release(pointer);
}

void operator delete[](void* pointer) noexcept {
    Release(pointer);
}

void operator delete(void* pointer, std::size_t /*size*/) noexcept {
    Release(pointer);
}

void operator delete[](void* pointer, std::size_t /*size*/) noexcept {
    Release(pointer);
}

void operator delete(void* pointer, const std::nothrow_t& /*tag*/) noexcept {
    Release(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t& /*tag*/) noexcept {
    Release(pointer);
}

void operator delete(void* pointer, std::align_val_t /*alignment*/) noexcept {
    ReleaseAligned(pointer);
}

void operator delete[](void* pointer, std::align_val_t /*alignment*/) noexcept {
    ReleaseAligned(pointer);
}

void operator delete(void* pointer, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    ReleaseAligned(pointer);
}

void operator delete[](void* pointer, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    ReleaseAligned(pointer);
}

void operator delete(void* pointer, std::align_val_t /*alignment*/, const std::nothrow_t& /*tag*/) noexcept {
    ReleaseAligned(pointer);
}

void operator delete[](void* pointer, std::align_val_t /*alignment*/,
                       const std::nothrow_t& /*tag*/) noexcept {
    ReleaseAligned(pointer);
}
