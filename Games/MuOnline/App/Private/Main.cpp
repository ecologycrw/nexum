#include <cstddef>
#include <cstdio>
#include <span>
#include <string_view>

#include "Mu/ModuleIdentity.hpp"

namespace {

#if NEXUM_AUTHORING_ENABLED
constexpr mu::BuildKind kBuildKind = mu::BuildKind::Development;
#else
constexpr mu::BuildKind kBuildKind = mu::BuildKind::Player;
#endif

} // namespace

int main(int argc, char* argv[]) {
    const std::span<char*> arguments(argv, static_cast<std::size_t>(argc));
    if (arguments.size() == 2 && std::string_view(arguments[1]) == "--version") {
        std::puts(mu::DescribeBuild(kBuildKind).c_str());
        return 0;
    }
    std::fputs("Usage: MuClient --version\n", stderr);
    return 2;
}
