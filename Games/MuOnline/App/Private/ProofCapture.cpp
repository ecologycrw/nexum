#include "ProofCapture.hpp"

#include <filesystem>
#include <string>

#include "ClientStart.hpp"
#include "Mu/Foundation/ProofScene.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Platform/Png.hpp"
#include "Nexum/Platform/SdlLogRoute.hpp"

namespace mu::app {

namespace core = nexum::core;

namespace {

constexpr nexum::debug::LogCategory kClient{"client"};

constexpr int kFailure = 1;

// The capture path is set: the command line refuses --capture without one.
[[nodiscard]] core::Result<void> CaptureToFile(const ClientOptions& options, nexum::debug::Logger& log) {
    // Declared in this order so they end in the reverse one: the device, the video and then the route of the
    // SDL messages.
    const nexum::platform::SdlLogRoute sdlLog(log);
    auto video = StartVideo(log);
    if (!video) {
        return tl::unexpected(video.error());
    }
    auto device = CreateDevice(*video, options, log);
    if (!device) {
        return tl::unexpected(device.error());
    }
    auto image = foundation::CaptureProofScene(*device, log);
    if (!image) {
        return tl::unexpected(image.error());
    }
    return nexum::platform::SavePng(*image, std::filesystem::path(*options.capturePath));
}

} // namespace

int RunProofCapture(const ClientOptions& options, nexum::debug::Logger& log) {
    if (auto captured = CaptureToFile(options, log); !captured) {
        log.Error(kClient, "could not capture the proof scene: " + captured.error().message);
        return kFailure;
    }
    log.Info(kClient, "wrote the proof scene to " + *options.capturePath);
    return 0;
}

} // namespace mu::app
