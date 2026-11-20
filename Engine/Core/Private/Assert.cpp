#include "Nexum/Core/Assert.hpp"

#include <cstdio>
#include <cstdlib>

namespace nexum::core {

void AssertFailed(const char* condition, const char* file, int line) noexcept {
    // Written straight to the error output, with no memory to ask for: the program may already be in a bad
    // state.
    std::fprintf(stderr, "NEXUM_ASSERT failed: %s at %s:%d\n", condition, file, line);
    std::fflush(stderr);
    std::abort();
}

} // namespace nexum::core
