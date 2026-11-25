// A test program of its own: Engine/Debug/Tests/CMakeLists.txt checks the lines it writes to the terminal.
#include <doctest/doctest.h>

#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Debug/TerminalLogSink.hpp"

using nexum::debug::LogCategory;
using nexum::debug::Logger;
using nexum::debug::TerminalLogSink;

TEST_CASE("[0000-CA-05] the terminal output writes one line per message, with its level and category") {
    TerminalLogSink sink;
    Logger logger{sink};
    logger.Warning(LogCategory{"render"}, "the swapchain was rebuilt");
}
