#include "Nexum/Platform/FileBytes.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "SdlFile.hpp"

namespace nexum::platform {

namespace {

[[nodiscard]] std::string ReadFailure(const std::string& file) {
    return "could not read the file " + file + ": " + SDL_GetError();
}

[[nodiscard]] std::string WriteFailure(const std::string& file) {
    return "could not write the file " + file + ": " + SDL_GetError();
}

// SDL may give fewer bytes than asked in one call, so the reading goes on until every byte is in.
[[nodiscard]] core::Result<void> ReadAll(SDL_IOStream* stream, std::span<std::byte> destination,
                                         const std::string& file) {
    std::size_t done = 0;
    while (done < destination.size()) {
        const std::span<std::byte> rest = destination.subspan(done);
        const std::size_t read = SDL_ReadIO(stream, rest.data(), rest.size());
        if (read == 0) {
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                return core::MakeError("the file " + file + " ended after " + std::to_string(done) +
                                       " of its " + std::to_string(destination.size()) + " bytes");
            }
            return core::MakeError(ReadFailure(file));
        }
        done += read;
    }
    return {};
}

} // namespace

core::Result<std::vector<std::byte>> ReadFileBytes(const std::filesystem::path& path, std::size_t limit) {
    const std::string file = ToUtf8(path);
    const Stream stream{SDL_IOFromFile(file.c_str(), "rb")};
    if (!stream) {
        return core::MakeError(ReadFailure(file));
    }
    const Sint64 size = SDL_GetIOSize(stream.get());
    if (size < 0) {
        return core::MakeError(ReadFailure(file));
    }
    const auto bytes = static_cast<std::uint64_t>(size);
    if (bytes > limit) {
        return core::MakeError("the file " + file + " has " + std::to_string(bytes) +
                               " bytes, more than the limit of " + std::to_string(limit));
    }
    std::vector<std::byte> content(static_cast<std::size_t>(bytes));
    auto read = ReadAll(stream.get(), content, file);
    if (!read) {
        return core::MakeError(std::move(read.error().message));
    }
    return content;
}

core::Result<void> WriteFileBytes(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    const std::string file = ToUtf8(path);
    Stream stream{SDL_IOFromFile(file.c_str(), "wb")};
    if (!stream) {
        return core::MakeError(WriteFailure(file));
    }
    if (SDL_WriteIO(stream.get(), bytes.data(), bytes.size()) != bytes.size()) {
        return core::MakeError(WriteFailure(file));
    }
    // Closing writes what SDL still holds, so the last bytes can fail here.
    if (!SDL_CloseIO(stream.release())) {
        return core::MakeError(WriteFailure(file));
    }
    return {};
}

} // namespace nexum::platform
