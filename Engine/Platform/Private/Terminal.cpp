#include "Nexum/Platform/Terminal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

#if defined(_WIN32)
// WIN32_LEAN_AND_MEAN and NOMINMAX come from Engine/Platform/CMakeLists.txt.
#include <windows.h>
#else
#include <cerrno>
#include <termios.h>
#include <unistd.h>
#endif

namespace nexum::platform {

namespace {

enum class ByteRead : std::uint8_t { Byte, End, Failure };

// Writes zeros over a copy of the secret. The writes go through volatile, so the compiler cannot drop them as
// stores that nothing reads afterwards.
template <typename Char>
void Erase(std::span<Char> text) noexcept {
    for (volatile Char& character : std::span<volatile Char>(text.data(), text.size())) {
        character = Char{};
    }
}

void Erase(std::string& text) noexcept {
    Erase(std::span<char>(text.data(), text.size()));
    text.clear();
}

[[nodiscard]] core::Error LineTooLong() {
    return core::Error{"the line is longer than " + std::to_string(kLongestHiddenLine) + " bytes"};
}

[[nodiscard]] core::Error EndedBeforeLine() {
    return core::Error{"the input ended before a line was typed"};
}

// The room of the text, reserved before the first byte, with one byte more for the "\r" of a "\r\n": it never
// grows, so the text never moves and leaves no copy behind. It is larger than the room inside a std::string,
// so the text lives in memory of its own, and a move of the string takes that memory along.
[[nodiscard]] std::string EmptyLine() {
    std::string line;
    line.reserve(kLongestHiddenLine + 1);
    return line;
}

// True when `next` does not fit after `line`: only the "\r" of a "\r\n" may follow the longest line.
[[nodiscard]] bool PassesTheLongest(const std::string& line, char next) noexcept {
    return line.size() > kLongestHiddenLine || (line.size() == kLongestHiddenLine && next != '\r');
}

// The bytes of one line, up to its end or the end of the input, from `readByte(char&)`, one at a time.
template <typename ReadByte>
[[nodiscard]] core::Result<std::string> ReadLine(ReadByte&& readByte) {
    std::string line = EmptyLine();
    bool readAny = false;
    std::array<char, 1> byte{};
    for (;;) {
        const ByteRead outcome = readByte(byte[0]);
        if (outcome == ByteRead::Failure) {
            Erase(line);
            return core::MakeError("could not read the standard input");
        }
        if (outcome == ByteRead::End || byte[0] == '\n') {
            readAny = readAny || outcome == ByteRead::Byte;
            break;
        }
        readAny = true;
        if (PassesTheLongest(line, byte[0])) {
            Erase(line);
            Erase(std::span<char>(byte));
            return tl::unexpected(LineTooLong());
        }
        line.push_back(byte[0]);
    }
    Erase(std::span<char>(byte));
    if (!readAny) {
        return tl::unexpected(EndedBeforeLine());
    }
    if (!line.empty() && line.back() == '\r') {
        line.back() = '\0';
        line.pop_back();
    }
    return line;
}

void WritePrompt(std::string_view prompt) noexcept {
    std::fwrite(prompt.data(), 1, prompt.size(), stderr);
    std::fflush(stderr);
}

[[nodiscard]] ByteRead ReadInputByte(char& byte) noexcept;

// The line of an input that is not a terminal. Nothing ended the line of the prompt, so the next output
// starts on a line of its own.
[[nodiscard]] core::Result<std::string> ReadPipedLine() {
    auto line = ReadLine(ReadInputByte);
    std::fputs("\n", stderr);
    return line;
}

#if defined(_WIN32)

[[nodiscard]] ByteRead ReadInputByte(char& byte) noexcept {
    DWORD count = 0;
    if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), &byte, 1, &count, nullptr) == FALSE) {
        // The writer of a pipe that closed is the end of the input.
        return GetLastError() == ERROR_BROKEN_PIPE ? ByteRead::End : ByteRead::Failure;
    }
    return count == 1 ? ByteRead::Byte : ByteRead::End;
}

// The console gives UTF-16, and the line goes out in UTF-8 (docs/CONVENTIONS.md, "Texto"). `typed` is what
// the console gave, with the end of the line.
[[nodiscard]] core::Result<std::string> ToUtf8(std::span<const wchar_t> typed) {
    if (typed.empty()) {
        return tl::unexpected(EndedBeforeLine());
    }
    if (typed.back() != L'\n') {
        // The console filled the room without reaching the end of the line.
        return tl::unexpected(LineTooLong());
    }
    std::size_t length = typed.size() - 1;
    if (length > 0 && typed[length - 1] == L'\r') {
        --length;
    }
    std::string line = EmptyLine();
    if (length == 0) {
        return line;
    }
    const auto wideLength = static_cast<int>(length);
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, typed.data(), wideLength, nullptr, 0,
                                         nullptr, nullptr);
    if (size <= 0) {
        return core::MakeError("the line is not valid text");
    }
    if (static_cast<std::size_t>(size) > kLongestHiddenLine) {
        return tl::unexpected(LineTooLong());
    }
    line.resize(static_cast<std::size_t>(size));
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, typed.data(), wideLength, line.data(), size, nullptr,
                        nullptr);
    return line;
}

// Turns the echo of the console off for one line, and back to what it was at the end. The Enter does not show
// either, so the next line of the terminal starts with an end of line.
[[nodiscard]] core::Result<std::string> ReadFromConsole(HANDLE input, DWORD mode) {
    const DWORD hidden =
        (mode & ~static_cast<DWORD>(ENABLE_ECHO_INPUT)) | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    if (SetConsoleMode(input, hidden) == FALSE) {
        return core::MakeError("the terminal cannot hide what is typed");
    }
    // Room for the longest line and its "\r\n", and one character more, which tells a line that is too long.
    std::array<wchar_t, kLongestHiddenLine + 3> typed{};
    DWORD count = 0;
    const BOOL typedLine =
        ReadConsoleW(input, typed.data(), static_cast<DWORD>(typed.size()), &count, nullptr);
    SetConsoleMode(input, mode);
    std::fputs("\n", stderr);
    core::Result<std::string> line =
        typedLine == FALSE ? core::Result<std::string>(core::MakeError("could not read the terminal"))
                           : ToUtf8(std::span<const wchar_t>(typed.data(), count));
    Erase(std::span<wchar_t>(typed));
    return line;
}

[[nodiscard]] core::Result<std::string> ReadInputLine() {
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(input, &mode) != FALSE) {
        return ReadFromConsole(input, mode);
    }
    return ReadPipedLine();
}

#else

[[nodiscard]] ByteRead ReadInputByte(char& byte) noexcept {
    for (;;) {
        const ssize_t count = read(STDIN_FILENO, &byte, 1);
        if (count == 1) {
            return ByteRead::Byte;
        }
        if (count == 0) {
            return ByteRead::End;
        }
        if (errno != EINTR) {
            return ByteRead::Failure;
        }
    }
}

// Turns the echo of the terminal off for one line, and back to what it was at the end. The end of the line
// still shows (ECHONL), so the next line of the terminal starts below the prompt.
[[nodiscard]] core::Result<std::string> ReadFromTerminal() {
    termios saved{};
    if (tcgetattr(STDIN_FILENO, &saved) != 0) {
        return core::MakeError("could not read the settings of the terminal");
    }
    termios hidden = saved;
    hidden.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    hidden.c_lflag |= static_cast<tcflag_t>(ECHONL);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) != 0) {
        return core::MakeError("the terminal cannot hide what is typed");
    }
    auto line = ReadLine(ReadInputByte);
    static_cast<void>(tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved));
    return line;
}

[[nodiscard]] core::Result<std::string> ReadInputLine() {
    if (isatty(STDIN_FILENO) != 0) {
        return ReadFromTerminal();
    }
    return ReadPipedLine();
}

#endif

} // namespace

core::Result<std::string> ReadHiddenLine(std::string_view prompt) {
    WritePrompt(prompt);
    return ReadInputLine();
}

} // namespace nexum::platform
