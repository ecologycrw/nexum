#include <doctest/doctest.h>

#include <cstddef>
#include <string_view>

#include "Nexum/Debug/Profiler.hpp"

// The text a macro expands to, as a string between brackets: "[]" when the macro expands to nothing. The
// brackets keep the text from being empty, because MSVC turns an empty text into no string at all.
#define NEXUM_TEST_TEXT_OF(...) #__VA_ARGS__
#define NEXUM_TEST_EXPANSION_OF(...) NEXUM_TEST_TEXT_OF([__VA_ARGS__])

namespace {

// Whether nothing but spaces is left between the brackets.
constexpr bool IsEmptyExpansion(std::string_view text) {
    const std::string_view inside = text.substr(1, text.size() - 2);
    return inside.find_first_not_of(' ') == std::string_view::npos;
}

} // namespace

TEST_CASE("[0000-CA-05] the profiler macros generate no code when the profiler is off") {
    constexpr std::string_view scope = NEXUM_TEST_EXPANSION_OF(NEXUM_PROFILE_SCOPE("tick"));
    constexpr std::string_view frame = NEXUM_TEST_EXPANSION_OF(NEXUM_PROFILE_FRAME());

    if constexpr (nexum::debug::kProfilerEnabled) {
        CHECK_FALSE(IsEmptyExpansion(scope));
        CHECK_FALSE(IsEmptyExpansion(frame));
    } else {
        CHECK(IsEmptyExpansion(scope));
        CHECK(IsEmptyExpansion(frame));
    }
}

#if NEXUM_PROFILER_ENABLED

namespace {

int ProfiledWork(int value) {
    NEXUM_PROFILE_SCOPE("profiled work");
    return value * 2;
}

int OtherProfiledWork(int value) {
    NEXUM_PROFILE_SCOPE("other profiled work");
    return value + 1;
}

} // namespace

// Without a Tracy viewer connected, the on-demand mode collects nothing, and the code around the scopes runs
// as usual. Each scope asks for its site only the first time it runs.
TEST_CASE("[0000-CA-05] with the profiler on, each profiled scope registers its site once") {
    const std::size_t sitesBefore = nexum::debug::ProfileSiteCount();

    int total = 0;
    for (int frame = 0; frame < 3; ++frame) {
        total += ProfiledWork(frame);
        total += OtherProfiledWork(frame);
        NEXUM_PROFILE_FRAME();
    }

    // ProfiledWork gives 0, 2 and 4; OtherProfiledWork gives 1, 2 and 3.
    CHECK(total == 12);
    CHECK(nexum::debug::ProfileSiteCount() == sitesBefore + 2);
}

#endif
