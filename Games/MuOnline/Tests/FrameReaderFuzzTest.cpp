// Fuzz target of the frame reader (cmake/NexumFuzzing.cmake): any bytes give messages or an error, never a
// crash; the bytes give the same messages, and the same error, whether they arrive at once or one byte at a
// time; and every message has the length its header declares.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

#include "Mu/Protocol/FrameReader.hpp"
#include "Mu/Protocol/PacketFields.hpp"

namespace {

struct Reading {
    std::vector<std::vector<std::byte>> messages;
    bool failed = false;
};

[[nodiscard]] Reading Read(std::span<const std::byte> bytes, std::size_t pieceSize) {
    mu::protocol::FrameReader reader;
    Reading reading;
    const auto keep = [&reading](std::span<const std::byte> message) {
        if (mu::protocol::DeclaredLength(message) != message.size()) {
            std::abort();
        }
        reading.messages.emplace_back(message.begin(), message.end());
    };
    for (std::size_t start = 0; start < bytes.size() && !reading.failed; start += pieceSize) {
        const auto piece = bytes.subspan(start, std::min(pieceSize, bytes.size() - start));
        reading.failed = !reader.Push(piece, keep).has_value();
    }
    return reading;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    const Reading whole = Read(bytes, bytes.empty() ? 1 : bytes.size());
    const Reading byByte = Read(bytes, 1);
    if (whole.failed != byByte.failed || whole.messages != byByte.messages) {
        std::abort();
    }
    return 0;
}
