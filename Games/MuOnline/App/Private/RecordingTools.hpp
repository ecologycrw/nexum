#pragma once

// The recording and the replay of a session in the MuClient, only in the development build (ADR 0035, and
// CA-04, CA-05 and CA-08 of specs/0002-gravacao-e-replay/spec.md). What --record and --replay ask for is made
// before the password, the connection and the window: the recording to play is read and checked whole, and
// the file of a new recording gets its header, so a path that cannot be written is refused before anything
// starts. The live server feeds the recorder during the frames; the recording ends with the last frame, and
// its file is written after the client ended, outside the frames (INV-06).

#include <memory>
#include <optional>
#include <string>

#include "Mu/App/ClientOptions.hpp"
#include "Mu/Online/OnlineClient.hpp"
#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/Recording/Recorder.hpp"
#include "Mu/Recording/Recording.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::app {

class RecordingTools {
public:
    // With --replay, reads and checks the recording; with --record, reserves the room of the recorder, 16
    // MiB, and writes the header to the file. The error says which file and why. `log` must outlive the
    // tools.
    [[nodiscard]] static nexum::core::Result<RecordingTools> Prepare(const ClientOptions& options,
                                                                     nexum::debug::Logger& log);

    // The tools only move: the server of the replay and the live server point into them once they are made.
    RecordingTools(const RecordingTools&) = delete;
    RecordingTools& operator=(const RecordingTools&) = delete;
    RecordingTools(RecordingTools&&) noexcept = default;
    RecordingTools& operator=(RecordingTools&&) noexcept = default;
    ~RecordingTools() = default;

    // The recorder for the live server, with --record; null without it.
    [[nodiscard]] recording::Recorder* Recorder() noexcept;

    // The server that plays the recording, with --replay; null without it. The tools must outlive it.
    [[nodiscard]] std::unique_ptr<online::ServerAdapter> MakeReplayServer() const;

    // The last frame ended, and the client applied the events that waited: the end of the recording, with the
    // view as the session left it, as failed when the session failed.
    void EndSession(const online::OnlineClient& client, nexum::time::Seconds now);

    // After the client ended: writes the whole recording to its file, and says so in the last line of the
    // log. Gives `exitCode`, or 1 when the file could not be written. Without --record, it only gives
    // `exitCode`.
    [[nodiscard]] int WriteRecording(int exitCode);

private:
    explicit RecordingTools(nexum::debug::Logger& log) noexcept : m_log(&log) {}

    nexum::debug::Logger* m_log;
    std::optional<recording::Recorder> m_recorder;
    std::string m_recordPath;
    std::optional<recording::Recording> m_replay;
};

} // namespace mu::app
