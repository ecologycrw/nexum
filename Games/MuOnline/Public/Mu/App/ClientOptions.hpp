#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Render/GpuBackend.hpp"

namespace mu::app {

// What the command line of MuClient asks for.
struct ClientOptions {
    // --version: write the version and the build kind, and end without opening a window.
    bool showVersion = false;
    // --frames N: end with success after N frames. Empty: run until the window is closed.
    std::optional<std::uint32_t> frameLimit;
    // --gpu-driver NAME: use only this backend (CA-06 of specs/0000-fundacao/spec.md). Empty: the order of
    // the system.
    std::optional<nexum::render::GpuBackend> gpuBackend;
};

// How to call MuClient, for the error output.
inline constexpr std::string_view kClientUsage =
    "Usage: MuClient [--frames N] [--gpu-driver vulkan|direct3d12|metal]\n"
    "       MuClient --version\n";

// Reads the command line (INV-12). The first argument is the name of the program. Each option comes at most
// once, --version comes alone, and the number of frames goes from 1 to 4294967295.
[[nodiscard]] nexum::core::Result<ClientOptions>
ParseClientOptions(std::span<const std::string_view> arguments);

} // namespace mu::app
