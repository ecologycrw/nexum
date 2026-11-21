#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "Mu/Foundation/FoundationScene.hpp"
#include "Mu/Foundation/FoundationScreen.hpp"
#include "Mu/Foundation/ProofScene.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TestCard.hpp"
#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/TestSupport/MemoryLogSink.hpp"
#include "Nexum/Time/Seconds.hpp"

using mu::foundation::DrawFoundation;
using mu::foundation::FoundationScreen;
using mu::foundation::FoundationSnapshot;
using mu::foundation::FoundationTextures;
using mu::foundation::kSquareColors;
using mu::foundation::kSurroundColor;
using nexum::render::DrawList;
using nexum::render::TargetSize;
using nexum::time::Seconds;

namespace {

// Handles that no store gave, which is enough for a draw list: only the drawing checks them.
FoundationTextures FakeTextures() {
    FoundationTextures textures;
    textures.card = nexum::render::TextureHandle{.index = 0, .generation = 1};
    for (std::uint32_t square = 0; square < textures.squares.size(); ++square) {
        textures.squares[square] = nexum::render::TextureHandle{.index = square + 1, .generation = 1};
    }
    return textures;
}

void CheckBlit(const nexum::render::Blit& blit, nexum::render::TextureHandle texture, std::int32_t x,
               std::int32_t y) {
    CHECK(blit.texture == texture);
    CHECK(blit.x == x);
    CHECK(blit.y == y);
}

// A pixel as text, such as "(12, 34, 56, 255)": doctest cannot print the type by itself.
std::string PixelText(const nexum::core::Rgba8& pixel) {
    return "(" + std::to_string(pixel.r) + ", " + std::to_string(pixel.g) + ", " + std::to_string(pixel.b) +
           ", " + std::to_string(pixel.a) + ")";
}

void CheckPixel(const nexum::core::Image& image, std::uint32_t x, std::uint32_t y,
                const nexum::core::Rgba8& expected) {
    const nexum::core::Rgba8 actual = image.At(x, y);
    CHECK_MESSAGE(actual == expected, "pixel (", x, ", ", y, ") is ", PixelText(actual), " instead of ",
                  PixelText(expected));
}

// What the GPU tests share, in the order the engine requires: video first, then the device and its store.
struct Gpu {
    nexum::platform::VideoSystem video;
    nexum::render::GpuDevice device;
    nexum::render::TextureStore store;
};

Gpu StartGpu(std::uint32_t textureCapacity) {
    auto video = nexum::platform::VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    auto device = nexum::render::GpuDevice::Create(*video, nexum::render::GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    auto store = nexum::render::TextureStore::Create(*device, textureCapacity);
    REQUIRE_MESSAGE(store.has_value(), (store ? "" : store.error().message));
    return Gpu{std::move(*video), std::move(*device), std::move(*store)};
}

} // namespace

TEST_CASE("the drawing puts the test card on a target of its size and the square over it") {
    const FoundationTextures textures = FakeTextures();
    DrawList list;
    REQUIRE(DrawFoundation(FoundationSnapshot{.squareX = 198, .squareY = 424, .squareColor = 2}, textures,
                           TargetSize{.width = 640, .height = 480}, list)
                .has_value());
    CHECK(list.ClearColor() == kSurroundColor);
    REQUIRE(list.Blits().size() == 2);
    CheckBlit(list.Blits()[0], textures.card, 0, 0);
    CheckBlit(list.Blits()[1], textures.squares[2], 198, 424);
}

TEST_CASE("the drawing keeps the test card in the middle of a larger target") {
    const FoundationTextures textures = FakeTextures();
    DrawList list;
    REQUIRE(DrawFoundation(FoundationSnapshot{.squareX = 4, .squareY = 424, .squareColor = 3}, textures,
                           TargetSize{.width = 1001, .height = 700}, list)
                .has_value());
    REQUIRE(list.Blits().size() == 2);
    CheckBlit(list.Blits()[0], textures.card, 180, 110);
    CheckBlit(list.Blits()[1], textures.squares[3], 184, 534);
}

TEST_CASE("the drawing cuts the same amount from each side of a smaller target") {
    const FoundationTextures textures = FakeTextures();
    DrawList list;
    REQUIRE(DrawFoundation(FoundationSnapshot{.squareX = 0, .squareY = 424, .squareColor = 0}, textures,
                           TargetSize{.width = 320, .height = 240}, list)
                .has_value());
    REQUIRE(list.Blits().size() == 2);
    CheckBlit(list.Blits()[0], textures.card, -160, -120);
    CheckBlit(list.Blits()[1], textures.squares[0], -160, 304);
}

TEST_CASE("each drawing replaces the copies of the frame before") {
    const FoundationTextures textures = FakeTextures();
    DrawList list;
    const FoundationSnapshot snapshot{.squareX = 0, .squareY = 424, .squareColor = 1};
    const TargetSize target{.width = 640, .height = 480};
    REQUIRE(DrawFoundation(snapshot, textures, target, list).has_value());
    REQUIRE(DrawFoundation(snapshot, textures, target, list).has_value());
    CHECK(list.Blits().size() == 2);
}

TEST_CASE("the foundation screen runs the ticks of each frame and draws the snapshot of the last one") {
    Gpu gpu = StartGpu(static_cast<std::uint32_t>(FoundationScreen::kTextureCount));
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log(sink);
    auto screen = FoundationScreen::Create(gpu.store, log, Seconds{0.0});
    REQUIRE_MESSAGE(screen.has_value(), (screen ? "" : screen.error().message));
    CHECK(gpu.store.Count() == FoundationScreen::kTextureCount);

    REQUIRE(mu::foundation::RunToTheProofScene(*screen).has_value());
    CHECK(screen->Ticks().ElapsedTicks() == 50);
    CHECK(screen->Ticks().Alpha() == doctest::Approx(0.5));
    CHECK(screen->Scene().Current().x == 200);

    const auto& blits = screen->Drawing().Blits();
    REQUIRE(blits.size() == 2);
    CHECK(blits[1].x == 198);
    CHECK(blits[1].y == 424);
}

TEST_CASE("[0000-CA-07] the proof scene shows the test card and the square at x 198 with the third color") {
    // The capture makes a store of its own: the one of the test stays unused.
    Gpu gpu = StartGpu(1);
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log(sink);
    auto image = mu::foundation::CaptureProofScene(gpu.device, log);
    REQUIRE_MESSAGE(image.has_value(), (image ? "" : image.error().message));
    REQUIRE(image->Width() == 640);
    REQUIRE(image->Height() == 480);

    // The square covers the columns 198 to 229 and the rows 424 to 455, and nothing around them.
    const nexum::core::Rgba8 third = kSquareColors[2];
    CheckPixel(*image, 198, 424, third);
    CheckPixel(*image, 229, 455, third);
    CheckPixel(*image, 197, 440, nexum::render::kTestCardBackground);
    CheckPixel(*image, 230, 440, nexum::render::kTestCardBackground);
    CheckPixel(*image, 210, 423, nexum::render::kTestCardBackground);
    CheckPixel(*image, 210, 456, nexum::render::kTestCardBackground);

    // Everything above the strip is the test card itself, pixel for pixel.
    const nexum::core::Image card = nexum::render::MakeTestCard();
    bool sameCard = true;
    for (std::uint32_t y = 0; y < nexum::render::kTestCardStripTop && sameCard; ++y) {
        for (std::uint32_t x = 0; x < card.Width() && sameCard; ++x) {
            if (image->At(x, y) != card.At(x, y)) {
                CheckPixel(*image, x, y, card.At(x, y));
                sameCard = false;
            }
        }
    }
    CHECK(sameCard);
}

TEST_CASE("the foundation screen writes the report of the tick loop to the log every 5 seconds") {
    Gpu gpu = StartGpu(static_cast<std::uint32_t>(FoundationScreen::kTextureCount));
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log(sink);
    auto screen = FoundationScreen::Create(gpu.store, log, Seconds{0.0});
    REQUIRE_MESSAGE(screen.has_value(), (screen ? "" : screen.error().message));

    // 11 seconds of frames of 40 ms: one tick each.
    const TargetSize target{.width = 640, .height = 480};
    for (int frame = 1; frame <= 275; ++frame) {
        REQUIRE(screen->Frame(Seconds{0.04 * frame}, Seconds{0.04}, target).has_value());
    }

    std::size_t reports = 0;
    for (const auto& entry : sink.Entries()) {
        if (entry.category == "time") {
            ++reports;
            CHECK(entry.level == nexum::debug::LogLevel::Info);
            CHECK(
                entry.message.starts_with("25.0 ticks per second, 0.00 ticks dropped, longest frame 40.0 ms, "
                                          "ticks up to 1/2/4/8 ms and above 8 ms: "));
        }
    }
    CHECK(reports == 2);
}

TEST_CASE("a store without room for every texture leaves the screen unmade and the store empty") {
    Gpu gpu = StartGpu(3);
    nexum::test_support::MemoryLogSink sink;
    nexum::debug::Logger log(sink);
    auto screen = FoundationScreen::Create(gpu.store, log, Seconds{0.0});
    REQUIRE_FALSE(screen.has_value());
    CHECK(screen.error().message.starts_with("could not send the square to the GPU: "));
    CHECK(gpu.store.Count() == 0);
}
