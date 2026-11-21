#pragma once

// The profiler macros of the engine (ADR 0018). The other modules mark their work with them and never see
// Tracy, which only Engine/Debug/Private uses (INV-10):
//
//     NEXUM_PROFILE_SCOPE("tick");   // measures from this line to the end of the block
//     NEXUM_PROFILE_FRAME();         // marks the end of a frame
//
// The name must be a string literal, because the profiler keeps its address until the program ends.
//
// The profiler is on only when the build is configured with NEXUM_PROFILER=ON, which the player build refuses
// (Engine/Debug/ProfilerOption.cmake). When it is off, the macros expand to nothing and generate no code.
// NEXUM_PROFILER_ENABLED comes from Engine/Debug/CMakeLists.txt.

#ifndef NEXUM_PROFILER_ENABLED
#error "NEXUM_PROFILER_ENABLED is missing: link the target to Nexum::Debug"
#endif

namespace nexum::debug {

// Whether this build has the profiler.
inline constexpr bool kProfilerEnabled = NEXUM_PROFILER_ENABLED != 0;

} // namespace nexum::debug

#if NEXUM_PROFILER_ENABLED

#include <cstddef>
#include <cstdint>

namespace nexum::debug {

// Where a profiled scope is in the code. Only Private/Profiler.cpp defines it: it holds what Tracy reads.
struct ProfileSite;

// Called by NEXUM_PROFILE_SCOPE the first time each scope runs. The strings and the site last until the
// program ends. After kMaxProfileSites sites, every new scope shares one site named after the limit.
[[nodiscard]] const ProfileSite& RegisterProfileSite(const char* name, const char* function, const char* file,
                                                     std::uint32_t line) noexcept;

inline constexpr std::size_t kMaxProfileSites = 4096;

// How many sites were registered so far, at most kMaxProfileSites.
[[nodiscard]] std::size_t ProfileSiteCount() noexcept;

// Measures the time from its creation to its destruction, while a Tracy viewer is connected.
class ProfileScope {
public:
    explicit ProfileScope(const ProfileSite& site) noexcept;
    ~ProfileScope();

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    ProfileScope(ProfileScope&&) = delete;
    ProfileScope& operator=(ProfileScope&&) = delete;

private:
    // What Tracy needs to close the zone.
    std::uint32_t m_zone = 0;
    std::int32_t m_active = 0;
    std::uint64_t m_connection = 0;
};

// Marks the end of a frame for the Tracy viewer.
void MarkProfileFrame() noexcept;

} // namespace nexum::debug

#define NEXUM_PROFILE_CONCAT_PARTS(left, right) left##right
#define NEXUM_PROFILE_CONCAT(left, right) NEXUM_PROFILE_CONCAT_PARTS(left, right)

#define NEXUM_PROFILE_SCOPE(name)                                                                            \
    static const ::nexum::debug::ProfileSite& NEXUM_PROFILE_CONCAT(nexumProfileSite, __LINE__) =             \
        ::nexum::debug::RegisterProfileSite(name, __func__, __FILE__, static_cast<std::uint32_t>(__LINE__)); \
    const ::nexum::debug::ProfileScope NEXUM_PROFILE_CONCAT(nexumProfileScope, __LINE__) {                   \
        NEXUM_PROFILE_CONCAT(nexumProfileSite, __LINE__)                                                     \
    }

#define NEXUM_PROFILE_FRAME() ::nexum::debug::MarkProfileFrame()

#else

#define NEXUM_PROFILE_SCOPE(name)
#define NEXUM_PROFILE_FRAME()

#endif
