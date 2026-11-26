#include "Nexum/Debug/Profiler.hpp"

// Without the profiler, this file has nothing to build: the macros expand to nothing.
#if NEXUM_PROFILER_ENABLED

#include <tracy/TracyC.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

// The plan of slice 0 asks for the on-demand mode: Tracy collects nothing until a viewer connects.
#ifndef TRACY_ON_DEMAND
#error "Tracy must be built with on-demand profiling: the \"on-demand\" feature in vcpkg.json"
#endif

namespace nexum::debug {

// The record of a profiled scope, which Tracy reads by its address while the program runs.
struct ProfileSite {
    ___tracy_source_location_data location;
};

namespace {

// The sites never move and are never destroyed, because Tracy may still read them while the program ends.
// Both objects are trivially destructible, so they need no destructor, and both start as constants, before
// any code. They are global because Tracy keeps the address of each site until the program ends, so no object
// of the engine can own them, and the profiling macros reach them from any function.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<ProfileSite, kMaxProfileSites> sites{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<std::size_t> siteCount{0};

constexpr ProfileSite kOverflowSite{{"nexum: more profiled scopes than kMaxProfileSites", "", __FILE__,
                                     static_cast<std::uint32_t>(__LINE__), 0}};

} // namespace

const ProfileSite& RegisterProfileSite(const char* name, const char* function, const char* file,
                                       std::uint32_t line) noexcept {
    const std::size_t index = siteCount.fetch_add(1, std::memory_order_relaxed);
    if (index >= kMaxProfileSites) {
        return kOverflowSite;
    }
    ProfileSite& site = sites[index];
    site.location = ___tracy_source_location_data{name, function, file, line, 0};
    return site;
}

std::size_t ProfileSiteCount() noexcept {
    return std::min(siteCount.load(std::memory_order_relaxed), kMaxProfileSites);
}

ProfileScope::ProfileScope(const ProfileSite& site) noexcept {
    const TracyCZoneCtx context = ___tracy_emit_zone_begin(&site.location, 1);
    m_zone = context.id;
    m_active = context.active;
    m_connection = context.connectionId;
}

ProfileScope::~ProfileScope() {
    TracyCZoneCtx context{};
    context.id = m_zone;
    context.active = m_active;
    context.connectionId = m_connection;
    ___tracy_emit_zone_end(context);
}

void MarkProfileFrame() noexcept {
    ___tracy_emit_frame_mark(nullptr);
}

} // namespace nexum::debug

#endif
