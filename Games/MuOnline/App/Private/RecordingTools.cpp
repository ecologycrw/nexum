#include "RecordingTools.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "Mu/Online/ReplayServer.hpp"
#include "Mu/Protocol/Dialect.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Core/Version.hpp"
#include "Nexum/Platform/FileBytes.hpp"

namespace mu::app {

namespace core = nexum::core;
using nexum::time::Seconds;

namespace {

constexpr nexum::debug::LogCategory kClient{"client"};

constexpr int kFailure = 1;
constexpr std::size_t kMebibyte = std::size_t{1024} * 1024;

// The header of a recording made by this client: the version of the Nexum, as --version gives it, the
// version of the dialect the client gives in the login, and the server, as --server-version names it.
[[nodiscard]] recording::RecordingHeader HeaderOf(const ClientOptions& options) {
    return recording::RecordingHeader{
        .nexumVersion = core::ToString(core::EngineVersion()),
        .dialectVersion = std::string(protocol::kClientIdentity.version),
        .server = options.serverVersion.value_or("unknown"),
    };
}

// "1 message", "2 messages".
[[nodiscard]] std::string Count(std::size_t count, std::string_view thing) {
    return std::to_string(count) + " " + std::string(thing) + (count == 1 ? "" : "s");
}

} // namespace

core::Result<RecordingTools> RecordingTools::Prepare(const ClientOptions& options,
                                                     nexum::debug::Logger& log) {
    RecordingTools tools(log);
    if (options.replayPath) {
        auto read = recording::ReadRecording(std::filesystem::path(*options.replayPath));
        if (!read) {
            return core::MakeError("could not read the recording " + *options.replayPath + ": " +
                                   read.error().message);
        }
        tools.m_replay.emplace(std::move(*read));
    }
    if (options.recordPath) {
        auto recorder = recording::Recorder::Create(HeaderOf(options), recording::kRecorderRoom, log);
        if (!recorder) {
            return core::MakeError("could not start the recording: " + recorder.error().message);
        }
        // The header goes to the file now, so a path that cannot be written stops the client before the
        // password and the connection. The whole recording replaces it at the end.
        if (auto written = nexum::platform::WriteFileBytes(std::filesystem::path(*options.recordPath),
                                                           recorder->Bytes());
            !written) {
            // The reason names the file.
            return core::MakeError("could not write the recording: " + written.error().message);
        }
        tools.m_recorder.emplace(std::move(*recorder));
        tools.m_recordPath = *options.recordPath;
    }
    return tools;
}

recording::Recorder* RecordingTools::Recorder() noexcept {
    return m_recorder ? &*m_recorder : nullptr;
}

std::unique_ptr<online::ServerAdapter> RecordingTools::MakeReplayServer() const {
    if (!m_replay) {
        return nullptr;
    }
    return std::make_unique<online::ReplayServer>(*m_replay, *m_log);
}

bool RecordingTools::EndSession(const online::OnlineClient& client, Seconds now) {
    const online::OnlineStatus status = client.Status(now);
    if (m_recorder) {
        m_recorder->Finish(client.View(), now,
                           status == online::OnlineStatus::Failed ? recording::EndReason::SessionFailed
                                                                  : recording::EndReason::Closed);
    }
    // A replay that failed already said why.
    if (m_replay && status != online::OnlineStatus::ServerEnded && status != online::OnlineStatus::Failed) {
        m_log->Error(kClient,
                     "the replay stopped before the end of the recording, so it checked only a part of it");
        return false;
    }
    return true;
}

int RecordingTools::WriteRecording(int exitCode) {
    if (!m_recorder) {
        return exitCode;
    }
    // A client that could not start never ended the recording, which has only its header: nothing came from
    // the world, so an empty view ends it without a checkpoint.
    if (!m_recorder->Ended()) {
        const world::WorldView empty{*m_log};
        m_recorder->Finish(empty, Seconds{0.0}, recording::EndReason::SessionFailed);
    }
    if (auto written =
            nexum::platform::WriteFileBytes(std::filesystem::path(m_recordPath), m_recorder->Bytes());
        !written) {
        m_log->Error(kClient, "could not write the recording: " + written.error().message);
        return kFailure;
    }
    std::string line = "wrote the recording to " + m_recordPath + ": " +
                       Count(m_recorder->Messages(), "message") + ", " + Count(m_recorder->Ticks(), "tick") +
                       " and " + Count(m_recorder->Checkpoints(), "checkpoint");
    if (m_recorder->Full()) {
        line +=
            ", until its room of " + std::to_string(recording::kRecorderRoom / kMebibyte) + " MiB filled up";
    }
    m_log->Info(kClient, line);
    return exitCode;
}

} // namespace mu::app
