#pragma once

#include "Mu/App/ClientOptions.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuDevice.hpp"

namespace mu::app {

// The start of the video and of the GPU, the same for the window of the client and for the capture of the
// proof scene. Each writes to the log what it started: the video driver, and then the GPU backend and the
// graphics card (CA-06 of specs/0000-fundacao/spec.md).
[[nodiscard]] nexum::core::Result<nexum::platform::VideoSystem> StartVideo(nexum::debug::Logger& log);

// The device uses the backend that --gpu-driver forces, or the order of the system. `video` must outlive it.
[[nodiscard]] nexum::core::Result<nexum::render::GpuDevice>
CreateDevice(const nexum::platform::VideoSystem& video, const ClientOptions& options,
             nexum::debug::Logger& log);

} // namespace mu::app
