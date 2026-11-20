#pragma once

#include <memory>

#include "Nexum/Debug/Logger.hpp"

namespace nexum::platform {

struct SdlLogForwarding;

// While it exists, the messages SDL writes to its own log go to the engine log instead (CA-05 of
// specs/0000-fundacao/spec.md), in the category "sdl" or in one per area of SDL, such as "sdl.gpu" and
// "sdl.video". When it starts, it announces itself through the log of SDL.
//
// Routes nest: the newest one gets the messages, and when it ends, the one before gets them back. They end in
// the reverse order they started, on the main thread. The logger must outlive the route.
class SdlLogRoute {
public:
    explicit SdlLogRoute(debug::Logger& logger);
    SdlLogRoute(const SdlLogRoute&) = delete;
    SdlLogRoute& operator=(const SdlLogRoute&) = delete;
    SdlLogRoute(SdlLogRoute&&) = delete;
    SdlLogRoute& operator=(SdlLogRoute&&) = delete;
    ~SdlLogRoute();

    // Whether SDL writes to this route now, and not to a newer one.
    [[nodiscard]] bool IsActive() const noexcept;

private:
    friend SdlLogForwarding;

    // The output SDL used before this route, given back when it ends. Its type comes from SDL, so it is
    // defined only in the source file.
    struct PreviousOutput;

    debug::Logger* m_logger;
    std::unique_ptr<PreviousOutput> m_previous;
};

} // namespace nexum::platform
