#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/Window.hpp"

namespace nexum::platform {

// What the program answers after each frame and each event: keep going, or end the process.
enum class AppStatus : std::uint8_t { Continue, Success, Failure };

enum class EventKind : std::uint8_t {
    // The system asks the program to end: Ctrl+C in the terminal, logging out, or RequestQuit.
    QuitRequested,
    // The drawing area of a window changed its size in pixels, which `Event::size` holds.
    WindowResized,
    WindowMinimized,
    // The window is back to its normal size, after being minimized or maximized.
    WindowRestored,
    // The user asked to close a window, for example with its close button, or the program called
    // Window::RequestClose.
    WindowCloseRequested,
};

// Something the system tells the program between two frames.
struct Event {
    EventKind kind = EventKind::QuitRequested;
    // The window of a window event; empty for QuitRequested. It may be a window the program already
    // destroyed.
    WindowId window;
    // The new size of the drawing area, in pixels, for WindowResized.
    PixelSize size;
};

// The program as the platform runs it: one call per frame and one per event, always on the thread that called
// RunApplication. The program is destroyed when it ends, before the platform stops.
class Application {
public:
    Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    virtual ~Application() = default;

    // One frame. The platform calls it again and again while it returns Continue.
    [[nodiscard]] virtual AppStatus Iterate() = 0;

    // One event. The events that arrived since the previous frame come before the next frame.
    [[nodiscard]] virtual AppStatus OnEvent(const Event& event) = 0;
};

// Creates the program, inside the platform, from the arguments of the command line. The first argument is the
// name of the program.
using ApplicationFactory =
    std::function<core::Result<std::unique_ptr<Application>>(std::span<const std::string_view> arguments)>;

// Runs a program through the main callbacks of SDL (plan of slice 0, item 5) and returns the exit code of the
// process: 0 when the program ends with Success, 1 when it ends with Failure. When the program cannot be
// created, the error goes to `log` and the exit code is 1. Only one program runs at a time: a call while
// another program runs writes an error and returns 1 at once. On Windows, the arguments come in the code page
// of the system, as main receives them (docs/TECH_DEBT.md, TD-0013).
[[nodiscard]] int RunApplication(int argc, char* argv[], const ApplicationFactory& factory,
                                 debug::Logger& log);

// Asks the running program to end: it receives QuitRequested between two frames and decides what to do.
[[nodiscard]] core::Result<void> RequestQuit();

} // namespace nexum::platform
