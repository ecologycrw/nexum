#include <doctest/doctest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/Application.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Platform/Window.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"

using nexum::core::MakeError;
using nexum::core::Result;
using nexum::debug::Logger;
using nexum::debug::LogLevel;
using nexum::platform::Application;
using nexum::platform::ApplicationFactory;
using nexum::platform::AppStatus;
using nexum::platform::Event;
using nexum::platform::EventKind;
using nexum::platform::RequestQuit;
using nexum::platform::RunApplication;
using nexum::platform::VideoSystem;
using nexum::platform::Window;
using nexum::platform::WindowOptions;
using nexum::test_support::MemoryLogSink;

namespace {

// A command line, as main receives it.
class CommandLine {
public:
    explicit CommandLine(std::vector<std::string> arguments) : m_arguments(std::move(arguments)) {
        for (std::string& argument : m_arguments) {
            m_pointers.push_back(argument.data());
        }
        m_pointers.push_back(nullptr);
    }

    [[nodiscard]] int Count() const noexcept { return static_cast<int>(m_arguments.size()); }

    [[nodiscard]] char** Values() noexcept { return m_pointers.data(); }

private:
    std::vector<std::string> m_arguments;
    std::vector<char*> m_pointers;
};

int Run(const ApplicationFactory& factory, Logger& log) {
    CommandLine commandLine({"nexum_platform_tests", "--frames", "3"});
    return RunApplication(commandLine.Count(), commandLine.Values(), factory, log);
}

// Counts its frames and ends after a fixed number of them, with the status it was given.
class CountingApp final : public Application {
public:
    CountingApp(int& frames, int lastFrame, AppStatus ending)
        : m_frames(frames), m_lastFrame(lastFrame), m_ending(ending) {}

    AppStatus Iterate() override {
        ++m_frames;
        return m_frames < m_lastFrame ? AppStatus::Continue : m_ending;
    }

    AppStatus OnEvent(const Event& /*event*/) override { return AppStatus::Continue; }

private:
    int& m_frames;
    int m_lastFrame;
    AppStatus m_ending;
};

// The longest wait for an event that the program asked for. The frames sleep a little, so a slow CI machine
// has time to deliver the event; a program that waits longer fails.
constexpr int kMaxWaitingFrames = 500;
constexpr auto kFrameSleep = std::chrono::milliseconds(10);

// Opens a window, does something to it on the first frame, and ends with success when the expected event
// arrives for its window. Other events of the window, such as the ones it sends when it opens, are ignored.
class WindowEventApp final : public Application {
public:
    using Action = Result<void> (*)(Window& window);
    using Expectation = bool (*)(const Event& event);

    WindowEventApp(VideoSystem video, Window window, Action action, Expectation expected,
                   std::optional<Event>& seen)
        : m_video(std::move(video)), m_window(std::move(window)), m_action(action), m_expected(expected),
          m_seen(seen) {}

    AppStatus Iterate() override {
        ++m_frames;
        if (m_frames == 1) {
            const Result<void> done = m_action(m_window);
            if (!done) {
                MESSAGE(done.error().message);
                return AppStatus::Failure;
            }
        }
        if (m_frames > kMaxWaitingFrames) {
            return AppStatus::Failure;
        }
        std::this_thread::sleep_for(kFrameSleep);
        return AppStatus::Continue;
    }

    AppStatus OnEvent(const Event& event) override {
        if (event.window == m_window.Id() && m_expected(event)) {
            m_seen = event;
            return AppStatus::Success;
        }
        return AppStatus::Continue;
    }

private:
    VideoSystem m_video;
    Window m_window;
    Action m_action;
    Expectation m_expected;
    std::optional<Event>& m_seen;
    int m_frames = 0;
};

Result<std::unique_ptr<Application>> MakeWindowEventApp(WindowEventApp::Action action,
                                                        WindowEventApp::Expectation expected,
                                                        std::optional<Event>& seen) {
    auto video = VideoSystem::Start();
    if (!video) {
        return tl::unexpected(video.error());
    }
    auto window =
        Window::Open(*video, WindowOptions{.title = "Nexum event test", .width = 320, .height = 240});
    if (!window) {
        return tl::unexpected(window.error());
    }
    return std::make_unique<WindowEventApp>(std::move(*video), std::move(*window), action, expected, seen);
}

} // namespace

TEST_CASE("the platform runs the frames of a program until it ends with success") {
    MemoryLogSink sink;
    Logger log{sink};
    int frames = 0;
    std::vector<std::string> arguments;

    const int exitCode = Run(
        [&](std::span<const std::string_view> given) -> Result<std::unique_ptr<Application>> {
            arguments.assign(given.begin(), given.end());
            return std::make_unique<CountingApp>(frames, 5, AppStatus::Success);
        },
        log);

    CHECK(exitCode == 0);
    CHECK(frames == 5);
    CHECK(arguments == std::vector<std::string>{"nexum_platform_tests", "--frames", "3"});
    CHECK(sink.Entries().empty());
}

TEST_CASE("a program that ends with a failure ends the process with exit code 1") {
    MemoryLogSink sink;
    Logger log{sink};
    int frames = 0;

    const int exitCode = Run(
        [&](std::span<const std::string_view> /*arguments*/) -> Result<std::unique_ptr<Application>> {
            return std::make_unique<CountingApp>(frames, 2, AppStatus::Failure);
        },
        log);

    CHECK(exitCode == 1);
    CHECK(frames == 2);
}

TEST_CASE("a program that cannot start ends the process with its error in the log") {
    MemoryLogSink sink;
    Logger log{sink};

    const int exitCode = Run(
        [](std::span<const std::string_view> /*arguments*/) -> Result<std::unique_ptr<Application>> {
            return MakeError("no GPU device");
        },
        log);

    CHECK(exitCode == 1);
    REQUIRE(sink.Entries().size() == 1);
    CHECK(sink.Entries()[0].level == LogLevel::Error);
    CHECK(sink.Entries()[0].category == "platform");
    CHECK(sink.Entries()[0].message == "the program could not start: no GPU device");
}

TEST_CASE("only one program runs at a time") {
    MemoryLogSink sink;
    Logger log{sink};
    int innerExitCode = -1;

    // The program tries to run a second program from its first frame, and then ends with success.
    class NestingApp final : public Application {
    public:
        NestingApp(Logger& log, int& innerExitCode) : m_log(log), m_innerExitCode(innerExitCode) {}

        AppStatus Iterate() override {
            m_innerExitCode = Run(
                [](std::span<const std::string_view> /*arguments*/) -> Result<std::unique_ptr<Application>> {
                    return MakeError("this program must never be created");
                },
                m_log);
            return AppStatus::Success;
        }

        AppStatus OnEvent(const Event& /*event*/) override { return AppStatus::Continue; }

    private:
        Logger& m_log;
        int& m_innerExitCode;
    };

    const int exitCode = Run(
        [&](std::span<const std::string_view> /*arguments*/) -> Result<std::unique_ptr<Application>> {
            return std::make_unique<NestingApp>(log, innerExitCode);
        },
        log);

    CHECK(exitCode == 0);
    CHECK(innerExitCode == 1);
    REQUIRE(sink.Entries().size() == 1);
    CHECK(sink.Entries()[0].message == "a program is already running: only one runs at a time");
}

TEST_CASE("[0000-CA-11] a request to close a window reaches the program as an event of that window") {
    MemoryLogSink sink;
    Logger log{sink};
    std::optional<Event> seen;

    const int exitCode = Run(
        [&](std::span<const std::string_view> /*arguments*/) {
            return MakeWindowEventApp(
                [](Window& window) { return window.RequestClose(); },
                [](const Event& event) { return event.kind == EventKind::WindowCloseRequested; }, seen);
        },
        log);

    for (const auto& entry : sink.Entries()) {
        MESSAGE(entry.message);
    }
    CHECK(exitCode == 0);
    REQUIRE(seen.has_value());
    CHECK(seen->kind == EventKind::WindowCloseRequested);
}

TEST_CASE("[0000-CA-11] a new size of a window reaches the program as its size in pixels") {
    MemoryLogSink sink;
    Logger log{sink};
    std::optional<Event> seen;

    const int exitCode = Run(
        [&](std::span<const std::string_view> /*arguments*/) {
            // The window also reports its first size when it opens; the test waits for the new one. A high
            // density screen has more pixels than points, never fewer.
            return MakeWindowEventApp([](Window& window) { return window.Resize(400, 300); },
                                      [](const Event& event) {
                                          return event.kind == EventKind::WindowResized &&
                                                 event.size.width >= 400 && event.size.height >= 300;
                                      },
                                      seen);
        },
        log);

    for (const auto& entry : sink.Entries()) {
        MESSAGE(entry.message);
    }
    CHECK(exitCode == 0);
    REQUIRE(seen.has_value());
    MESSAGE("new drawing area: ", seen->size.width, "x", seen->size.height, " pixels");
    CHECK(seen->kind == EventKind::WindowResized);
}

TEST_CASE("a request to end reaches the program as QuitRequested") {
    MemoryLogSink sink;
    Logger log{sink};
    bool quitSeen = false;

    // Asks to end on the first frame, and ends with success when the request arrives.
    class QuittingApp final : public Application {
    public:
        explicit QuittingApp(bool& quitSeen) : m_quitSeen(quitSeen) {}

        AppStatus Iterate() override {
            ++m_frames;
            if (m_frames == 1 && !RequestQuit()) {
                return AppStatus::Failure;
            }
            return m_frames > kMaxWaitingFrames ? AppStatus::Failure : AppStatus::Continue;
        }

        AppStatus OnEvent(const Event& event) override {
            if (event.kind != EventKind::QuitRequested) {
                return AppStatus::Continue;
            }
            m_quitSeen = true;
            return AppStatus::Success;
        }

    private:
        bool& m_quitSeen;
        int m_frames = 0;
    };

    const int exitCode = Run(
        [&](std::span<const std::string_view> /*arguments*/) -> Result<std::unique_ptr<Application>> {
            return std::make_unique<QuittingApp>(quitSeen);
        },
        log);

    CHECK(exitCode == 0);
    CHECK(quitSeen);
}
