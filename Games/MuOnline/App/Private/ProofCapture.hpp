#pragma once

#include "Mu/App/ClientOptions.hpp"
#include "Nexum/Debug/Logger.hpp"

namespace mu::app {

// MuClient --capture FILE (CA-07 of specs/0000-fundacao/spec.md): starts the video and the GPU without any
// window, draws the proof scene and writes it to the PNG file. Returns the exit code of the client: 0 when
// the file was written, 1 otherwise, with the reason in the log. Only the development build has it (INV-19).
[[nodiscard]] int RunProofCapture(const ClientOptions& options, nexum::debug::Logger& log);

} // namespace mu::app
