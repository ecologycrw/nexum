#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "Nexum/Core/Handle.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Render/GpuDevice.hpp"

namespace nexum::render {

// A texture of the GPU. Only this module sees what is inside; the rest of the engine holds a TextureHandle.
struct Texture;

// A reference to a texture of a TextureStore (III.6 of the reference spec): once the texture is released, the
// handle is refused, even when its place holds another texture.
using TextureHandle = core::Handle<Texture>;

struct TextureSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    friend bool operator==(const TextureSize&, const TextureSize&) = default;
};

// The textures of one GPU device, each in RGBA with 8 bits per channel, the order of core::Image. The store
// holds a fixed number of them and never grows. It must be destroyed before its device.
class TextureStore {
public:
    // The largest side Direct3D 12 guarantees for a texture.
    static constexpr std::uint32_t kMaxSide = 16384;

    [[nodiscard]] static core::Result<TextureStore> Create(GpuDevice& device, std::uint32_t capacity);

    TextureStore(TextureStore&& other) noexcept;
    TextureStore& operator=(TextureStore&& other) noexcept;
    TextureStore(const TextureStore&) = delete;
    TextureStore& operator=(const TextureStore&) = delete;
    ~TextureStore();

    // Sends an image to the GPU as a new texture. The GPU has the pixels before it runs any later frame. An
    // empty image, a side above kMaxSide and a full store are refused.
    [[nodiscard]] core::Result<TextureHandle> Upload(const core::Image& image);

    // Releases the texture. Returns false, and changes nothing, when the handle is refused. A frame already
    // sent to the GPU still draws the texture: the GPU frees it afterwards.
    bool Release(TextureHandle texture) noexcept;

    // The size of the texture, or nothing when the handle is refused.
    [[nodiscard]] std::optional<TextureSize> SizeOf(TextureHandle texture) const noexcept;

    [[nodiscard]] std::uint32_t Count() const noexcept;
    [[nodiscard]] std::uint32_t Capacity() const noexcept;

private:
    struct State;
    friend struct TextureStoreAccess;

    explicit TextureStore(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render
