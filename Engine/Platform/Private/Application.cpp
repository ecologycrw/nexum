#include "Nexum/Platform/Application.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
// Only the declarations of SDL_RunApp and SDL_EnterAppMainCallbacks: the main function is the program's own.
// SDL_MAIN_NOIMPL comes from Engine/Platform/CMakeLists.txt.
#include <SDL3/SDL_main.h>

namespace nexum::platform {

namespace {

constexpr debug::LogCategory kPlatform{"platform"};

// What RunApplication hands to the callbacks of SDL, which take only plain functions. It lives on the stack
// of RunApplication, and only while it runs.
struct Launch {
    const ApplicationFactory* factory = nullptr;
    debug::Logger* log = nullptr;
    std::unique_ptr<Application> application;
};

Launch* currentLaunch = nullptr;

[[nodiscard]] SDL_AppResult ToSdl(AppStatus status) noexcept {
    switch (status) {
    case AppStatus::Continue:
        return SDL_APP_CONTINUE;
    case AppStatus::Success:
        return SDL_APP_SUCCESS;
    case AppStatus::Failure:
        return SDL_APP_FAILURE;
    }
    return SDL_APP_FAILURE;
}

[[nodiscard]] std::uint32_t ToUnsigned(Sint32 value) noexcept {
    return value > 0 ? static_cast<std::uint32_t>(value) : 0;
}

[[nodiscard]] Event WindowEvent(EventKind kind, const SDL_WindowEvent& window) noexcept {
    return Event{.kind = kind, .window = WindowId{.value = window.windowID}, .size = PixelSize{}};
}

// The events the program learns about. The others are ignored until a slice needs them (keyboard, mouse...).
// SDL sends WINDOW_RESIZED in points and WINDOW_PIXEL_SIZE_CHANGED in pixels; the GPU draws pixels, and the
// pixel size also changes when the window moves to a screen of another density.
[[nodiscard]] std::optional<Event> Translate(const SDL_Event& event) noexcept {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        return Event{.kind = EventKind::QuitRequested, .window = WindowId{}, .size = PixelSize{}};
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        Event resized = WindowEvent(EventKind::WindowResized, event.window);
        resized.size =
            PixelSize{.width = ToUnsigned(event.window.data1), .height = ToUnsigned(event.window.data2)};
        return resized;
    }
    case SDL_EVENT_WINDOW_MINIMIZED:
        return WindowEvent(EventKind::WindowMinimized, event.window);
    case SDL_EVENT_WINDOW_RESTORED:
        return WindowEvent(EventKind::WindowRestored, event.window);
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return WindowEvent(EventKind::WindowCloseRequested, event.window);
    default:
        return std::nullopt;
    }
}

SDL_AppResult SDLCALL InitCallback(void** appState, int argc, char* argv[]) {
    *appState = nullptr;
    const std::span<char*> given(argv, static_cast<std::size_t>(argc > 0 ? argc : 0));
    const std::vector<std::string_view> arguments(given.begin(), given.end());

    auto created = (*currentLaunch->factory)(arguments);
    if (!created) {
        currentLaunch->log->Error(kPlatform, "the program could not start: " + created.error().message);
        return SDL_APP_FAILURE;
    }
    if (!*created) {
        currentLaunch->log->Error(kPlatform, "the program could not start: the factory gave no program");
        return SDL_APP_FAILURE;
    }
    currentLaunch->application = std::move(*created);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL IterateCallback(void* /*appState*/) {
    return ToSdl(currentLaunch->application->Iterate());
}

SDL_AppResult SDLCALL EventCallback(void* /*appState*/, SDL_Event* event) {
    const std::optional<Event> translated = Translate(*event);
    if (!translated) {
        return SDL_APP_CONTINUE;
    }
    return ToSdl(currentLaunch->application->OnEvent(*translated));
}

// SDL calls it once at the end, also when the program could not start. Right after it, SDL stops itself.
void SDLCALL QuitCallback(void* /*appState*/, SDL_AppResult /*result*/) {
    currentLaunch->application.reset();
}

int SDLCALL MainFunction(int argc, char* argv[]) {
    return SDL_EnterAppMainCallbacks(argc, argv, InitCallback, IterateCallback, EventCallback, QuitCallback);
}

} // namespace

int RunApplication(int argc, char* argv[], const ApplicationFactory& factory, debug::Logger& log) {
    if (currentLaunch != nullptr) {
        log.Error(kPlatform, "a program is already running: only one runs at a time");
        return 1;
    }
    Launch launch{.factory = &factory, .log = &log, .application = nullptr};
    currentLaunch = &launch;
    // SDL_RunApp does what the main function of SDL would do before the callbacks on each system.
    const int exitCode = SDL_RunApp(argc, argv, MainFunction, nullptr);
    currentLaunch = nullptr;
    return exitCode;
}

core::Result<void> RequestQuit() {
    SDL_Event event{};
    event.quit.type = SDL_EVENT_QUIT;
    if (!SDL_PushEvent(&event)) {
        return core::MakeError(std::string("could not ask the program to end: ") + SDL_GetError());
    }
    return {};
}

} // namespace nexum::platform
