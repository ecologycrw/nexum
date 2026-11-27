// Reads one hidden line, as ReadHiddenLine gives it, and writes its bytes in hex, so the tests of
// CheckHiddenLine.cmake see exactly what was read, without the end of the line. The line it reads is a test
// value, never a real secret.
#include <cstdio>
#include <string>
#include <string_view>

#include "Nexum/Platform/Terminal.hpp"

int main() {
    const auto line = nexum::platform::ReadHiddenLine("Secret: ");
    if (!line) {
        std::fputs(("error: " + line.error().message + "\n").c_str(), stdout);
        return 1;
    }
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::string text = "read " + std::to_string(line->size()) + " bytes:";
    for (const char character : *line) {
        const auto value = static_cast<unsigned char>(character);
        text += ' ';
        text += kDigits[value >> 4U];
        text += kDigits[value & 0x0FU];
    }
    text += '\n';
    std::fputs(text.c_str(), stdout);
    return 0;
}
