#include "Nexum/Platform/SdlLogRoute.hpp"

#include <memory>
#include <string_view>

#include <SDL3/SDL.h>

namespace nexum::platform {

namespace {

[[nodiscard]] debug::LogCategory CategoryOf(int category) noexcept {
    switch (category) {
    case SDL_LOG_CATEGORY_ERROR:
        return {"sdl.error"};
    case SDL_LOG_CATEGORY_ASSERT:
        return {"sdl.assert"};
    case SDL_LOG_CATEGORY_SYSTEM:
        return {"sdl.system"};
    case SDL_LOG_CATEGORY_AUDIO:
        return {"sdl.audio"};
    case SDL_LOG_CATEGORY_VIDEO:
        return {"sdl.video"};
    case SDL_LOG_CATEGORY_RENDER:
        return {"sdl.render"};
    case SDL_LOG_CATEGORY_INPUT:
        return {"sdl.input"};
    case SDL_LOG_CATEGORY_TEST:
        return {"sdl.test"};
    case SDL_LOG_CATEGORY_GPU:
        return {"sdl.gpu"};
    default:
        return {"sdl"};
    }
}

// The priorities of SDL grow with how serious the message is, like the levels of the engine.
[[nodiscard]] debug::LogLevel LevelOf(SDL_LogPriority priority) noexcept {
    if (priority >= SDL_LOG_PRIORITY_ERROR) {
        return debug::LogLevel::Error;
    }
    if (priority == SDL_LOG_PRIORITY_WARN) {
        return debug::LogLevel::Warning;
    }
    if (priority == SDL_LOG_PRIORITY_INFO) {
        return debug::LogLevel::Info;
    }
    if (priority == SDL_LOG_PRIORITY_DEBUG) {
        return debug::LogLevel::Debug;
    }
    return debug::LogLevel::Trace;
}

} // namespace

// Reaches the private part of the route for the output function SDL calls.
struct SdlLogForwarding {
    static void SDLCALL Output(void* context, int category, SDL_LogPriority priority, const char* message) {
        const auto* const route = static_cast<const SdlLogRoute*>(context);
        const std::string_view text = message != nullptr ? std::string_view(message) : std::string_view();
        route->m_logger->Write(LevelOf(priority), CategoryOf(category), text);
    }
};

struct SdlLogRoute::PreviousOutput {
    SDL_LogOutputFunction output = nullptr;
    void* context = nullptr;
};

SdlLogRoute::SdlLogRoute(debug::Logger& logger)
    : m_logger(&logger), m_previous(std::make_unique<PreviousOutput>()) {
    SDL_GetLogOutputFunction(&m_previous->output, &m_previous->context);
    SDL_SetLogOutputFunction(&SdlLogForwarding::Output, this);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "the messages of SDL now go to the engine log");
}

SdlLogRoute::~SdlLogRoute() {
    if (IsActive()) {
        SDL_SetLogOutputFunction(m_previous->output, m_previous->context);
        return;
    }
    // A newer route keeps this one as the output it gives back. Walk the newer routes, from the newest, and
    // hand this route's own previous output to the one that kept it, so no route gives back a route that
    // ended.
    SDL_LogOutputFunction output = nullptr;
    void* context = nullptr;
    SDL_GetLogOutputFunction(&output, &context);
    while (output == &SdlLogForwarding::Output && context != nullptr) {
        PreviousOutput& newerPrevious = *static_cast<SdlLogRoute*>(context)->m_previous;
        if (newerPrevious.output == &SdlLogForwarding::Output && newerPrevious.context == this) {
            newerPrevious = *m_previous;
            return;
        }
        output = newerPrevious.output;
        context = newerPrevious.context;
    }
}

bool SdlLogRoute::IsActive() const noexcept {
    SDL_LogOutputFunction output = nullptr;
    void* context = nullptr;
    SDL_GetLogOutputFunction(&output, &context);
    return output == &SdlLogForwarding::Output && context == static_cast<const void*>(this);
}

} // namespace nexum::platform
