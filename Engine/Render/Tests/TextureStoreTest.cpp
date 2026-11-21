#include <doctest/doctest.h>

#include <cstdint>
#include <utility>

#include "GpuFixture.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/TextureStore.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using nexum::render::TextureHandle;
using nexum::render::TextureSize;
using nexum::render::TextureStore;
using nexum::render_tests::GpuFixture;
using nexum::render_tests::StartGpu;

namespace {

TextureStore MakeStore(GpuFixture& gpu, std::uint32_t capacity) {
    auto store = TextureStore::Create(gpu.device, capacity);
    REQUIRE_MESSAGE(store.has_value(), (store ? "" : store.error().message));
    return std::move(*store);
}

TextureHandle UploadOrFail(TextureStore& store, const Image& image) {
    auto texture = store.Upload(image);
    REQUIRE_MESSAGE(texture.has_value(), (texture ? "" : texture.error().message));
    return *texture;
}

} // namespace

TEST_CASE("[0000-CA-04] an uploaded texture keeps its size until it is released") {
    GpuFixture gpu = StartGpu();
    TextureStore store = MakeStore(gpu, 4);
    CHECK(store.Capacity() == 4);
    CHECK(store.Count() == 0);

    const TextureHandle texture = UploadOrFail(store, Image(8, 4, Rgba8{.r = 1, .g = 2, .b = 3, .a = 255}));
    CHECK(store.Count() == 1);
    REQUIRE(store.SizeOf(texture).has_value());
    CHECK(*store.SizeOf(texture) == TextureSize{.width = 8, .height = 4});

    CHECK(store.Release(texture));
    CHECK(store.Count() == 0);
    CHECK_FALSE(store.SizeOf(texture).has_value());
    CHECK_FALSE(store.Release(texture));
}

TEST_CASE("[0000-CA-04] the handle of a released texture is refused after its place holds a new texture") {
    GpuFixture gpu = StartGpu();
    TextureStore store = MakeStore(gpu, 1);
    const TextureHandle old = UploadOrFail(store, Image(2, 2));
    REQUIRE(store.Release(old));

    const TextureHandle current = UploadOrFail(store, Image(3, 5));
    CHECK(current.index == old.index);
    CHECK(current != old);
    CHECK_FALSE(store.SizeOf(old).has_value());
    CHECK(*store.SizeOf(current) == TextureSize{.width = 3, .height = 5});
}

TEST_CASE("[0000-CA-04] a full texture store refuses a new texture until one is released") {
    GpuFixture gpu = StartGpu();
    TextureStore store = MakeStore(gpu, 2);
    const TextureHandle first = UploadOrFail(store, Image(1, 1));
    UploadOrFail(store, Image(1, 1));

    const auto refused = store.Upload(Image(1, 1));
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the texture store is full (2 textures)");
    CHECK(store.Count() == 2);

    REQUIRE(store.Release(first));
    CHECK(store.Upload(Image(1, 1)).has_value());
}

TEST_CASE("a texture store refuses an empty or oversized image, and a store with no room") {
    GpuFixture gpu = StartGpu();
    TextureStore store = MakeStore(gpu, 2);
    for (const auto& [width, height] :
         {std::pair<std::uint32_t, std::uint32_t>{0, 0},
          std::pair<std::uint32_t, std::uint32_t>{TextureStore::kMaxSide + 1, 1}}) {
        CAPTURE(width);
        CAPTURE(height);
        const auto refused = store.Upload(Image(width, height));
        REQUIRE_FALSE(refused.has_value());
        CHECK(
            refused.error().message.starts_with("a texture must be between 1 and 16384 pixels on each side"));
    }
    CHECK(store.Count() == 0);

    const auto empty = TextureStore::Create(gpu.device, 0);
    REQUIRE_FALSE(empty.has_value());
    CHECK(empty.error().message == "a texture store needs room for at least one texture");
}
