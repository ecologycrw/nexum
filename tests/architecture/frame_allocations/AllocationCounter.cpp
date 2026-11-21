// Replaces the global allocation functions of C++ for the test program that links the counter, to count the
// requests for memory (CA-09 of specs/0000-fundacao/spec.md). The cpp-rules check allows this only in this
// folder.

#include "AllocationCounter.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <stdlib.h>
#endif

namespace {

// What the calling thread counts. It is initialized as a constant, so an allocation function can read it even
// while a new thread is still starting.
struct ThreadCount {
    bool counting = false;
    frame_allocations::AllocationTally tally;
};

thread_local constinit ThreadCount tCount;

void Record(std::size_t size) noexcept {
    if (tCount.counting) {
        ++tCount.tally.requests;
        tCount.tally.bytes += size;
    }
}

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
    tCount.tally = AllocationTally{};
    tCount.counting = true;
}

AllocationCounter::~AllocationCounter() {
    tCount.counting = false;
}

AllocationTally AllocationCounter::Tally() const noexcept {
    return tCount.tally;
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
