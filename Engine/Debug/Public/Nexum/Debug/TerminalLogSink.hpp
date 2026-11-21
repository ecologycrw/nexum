#pragma once

#include "Nexum/Debug/Logger.hpp"

namespace nexum::debug {

// Writes each message as one line to the error output of the terminal: "[warning] render: the message". For
// now the log only goes to the terminal; the log file comes with the user folder (docs/TECH_DEBT.md, M7).
// Each line is written in a single call, so lines from different threads do not mix.
class TerminalLogSink final : public LogSink {
public:
    void Write(const LogRecord& record) noexcept override;
};

} // namespace nexum::debug
