#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "Mu/ModuleIdentity.hpp"
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
    // --capture FILE: write the proof scene to this PNG file without opening a window, and end (CA-07). Only
    // the development build has it.
    std::optional<std::string> capturePath;
};

// How to call MuClient in this kind of build, for the error output.
[[nodiscard]] std::string_view ClientUsage(BuildKind build) noexcept;

// Reads the command line (INV-12). The first argument is the name of the program. Each option comes at most
// once, --version comes alone, the number of frames goes from 1 to 4294967295, and --capture, which draws
// without a window, comes without --frames. The player build refuses --capture (INV-19).
[[nodiscard]] nexum::core::Result<ClientOptions>
ParseClientOptions(std::span<const std::string_view> arguments, BuildKind build);

} // namespace mu::app
