#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/Png.hpp"
#include "VisualProof/ImageComparison.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using visualproof::Approval;
using visualproof::CheckHasContent;
using visualproof::CompareFiles;
using visualproof::CompareImages;
using visualproof::Tolerance;

namespace {

// A folder of its own inside the build folder, where CTest runs the tests, emptied at the start of each test.
std::filesystem::path FreshFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::current_path() / "visualproof_test_output" / name;
    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    std::filesystem::create_directories(folder);
    return folder;
}

// 4 by 3 pixels unless asked otherwise, every one of them different, with every channel used, the alpha
// included.
Image MakeScene(std::uint32_t width = 4, std::uint32_t height = 3) {
    Image image(width, height);
    for (std::uint32_t y = 0; y < image.Height(); ++y) {
        for (std::uint32_t x = 0; x < image.Width(); ++x) {
            image.At(x, y) = Rgba8{.r = static_cast<std::uint8_t>(20 * x + 1),
                                   .g = static_cast<std::uint8_t>(30 * y + 1),
                                   .b = static_cast<std::uint8_t>(10 * (x + y) + 1),
                                   .a = static_cast<std::uint8_t>(250 - x - y)};
        }
    }
    return image;
}

std::string ErrorOf(const nexum::core::Result<Approval>& result) {
    REQUIRE_FALSE(result.has_value());
    return result.error().message;
}

} // namespace

TEST_CASE("[0000-CA-08] visualproof approves two equal images, even with no tolerance") {
    const auto approval = CompareImages(MakeScene(), MakeScene(), Tolerance{.levels = 0});
    REQUIRE(approval.has_value());
    CHECK(approval->pixels == 12);
    CHECK(approval->largestDifference == 0);
}

TEST_CASE("[0000-CA-08] visualproof approves a capture that differs by at most the tolerance") {
    Image capture = MakeScene();
    capture.At(0, 0).r = static_cast<std::uint8_t>(capture.At(0, 0).r + 1);
    capture.At(3, 2).g = static_cast<std::uint8_t>(capture.At(3, 2).g - 1);
    capture.At(1, 2).a = static_cast<std::uint8_t>(capture.At(1, 2).a + 1);
    const auto approval = CompareImages(capture, MakeScene(), Tolerance{.levels = 1});
    REQUIRE(approval.has_value());
    CHECK(approval->pixels == 12);
    CHECK(approval->largestDifference == 1);
}

TEST_CASE("[0000-CA-08] visualproof refuses images of different sizes") {
    CHECK(ErrorOf(CompareImages(MakeScene(5, 3), MakeScene(), Tolerance{.levels = 1})) ==
          "the capture is 5x3 pixels, but the approved image is 4x3 pixels");
    CHECK(ErrorOf(CompareImages(MakeScene(), MakeScene(4, 4), Tolerance{.levels = 255})) ==
          "the capture is 4x3 pixels, but the approved image is 4x4 pixels");
    // The same number of pixels in another shape is still another size.
    CHECK(ErrorOf(CompareImages(MakeScene(3, 4), MakeScene(), Tolerance{.levels = 255})) ==
          "the capture is 3x4 pixels, but the approved image is 4x3 pixels");
}

TEST_CASE("[0000-CA-08] visualproof refuses a single pixel beyond the tolerance, in any channel") {
    Image capture = MakeScene();
    capture.At(2, 1).b = static_cast<std::uint8_t>(capture.At(2, 1).b + 2);
    CHECK(ErrorOf(CompareImages(capture, MakeScene(), Tolerance{.levels = 1})) ==
          "1 of 12 pixels is beyond the tolerance of 1 level; the first is at (2, 1): (41, 31, 33, 247) "
          "instead of (41, 31, 31, 247)");

    Image transparent = MakeScene();
    transparent.At(0, 0).a = 0;
    CHECK(ErrorOf(CompareImages(transparent, MakeScene(), Tolerance{.levels = 200})) ==
          "1 of 12 pixels is beyond the tolerance of 200 levels; the first is at (0, 0): (1, 1, 1, 0) "
          "instead of (1, 1, 1, 250)");
}

TEST_CASE("visualproof counts every pixel beyond the tolerance and shows the first one, row by row") {
    Image capture = MakeScene();
    capture.At(3, 2).r = 0;
    capture.At(1, 1).g = 0;
    capture.At(2, 1).r = 0;
    CHECK(ErrorOf(CompareImages(capture, MakeScene(), Tolerance{.levels = 0})) ==
          "3 of 12 pixels are beyond the tolerance of 0 levels; the first is at (1, 1): (21, 0, 21, 248) "
          "instead of (21, 31, 21, 248)");
}

TEST_CASE("[0000-CA-08] visualproof refuses a capture of a single color, whatever it is compared with") {
    const Image black(4, 3, Rgba8{.r = 0, .g = 0, .b = 0, .a = 255});
    CHECK(ErrorOf(CompareImages(black, black, Tolerance{.levels = 1})) ==
          "the capture has a single color, (0, 0, 0, 255): an empty, black or single-color image fails "
          "(docs/VISUAL.md)");
    CHECK(ErrorOf(CompareImages(MakeScene(), Image(4, 3, Rgba8{.r = 7, .g = 8, .b = 9, .a = 10}),
                                Tolerance{.levels = 255})) ==
          "the approved image has a single color, (7, 8, 9, 10): an empty, black or single-color image fails "
          "(docs/VISUAL.md)");
}

TEST_CASE("an image without pixels has no content") {
    const auto content = CheckHasContent(Image(), "capture");
    REQUIRE_FALSE(content.has_value());
    CHECK(content.error().message == "the capture has no pixels");
    CHECK(CheckHasContent(MakeScene(), "capture").has_value());
}

TEST_CASE("visualproof compares two PNG files") {
    const std::filesystem::path folder = FreshFolder("files");
    Image capture = MakeScene();
    capture.At(1, 0).g = static_cast<std::uint8_t>(capture.At(1, 0).g + 1);
    REQUIRE(nexum::platform::SavePng(capture, folder / "capture.png").has_value());
    REQUIRE(nexum::platform::SavePng(MakeScene(), folder / "approved.png").has_value());

    const auto approval =
        CompareFiles(folder / "capture.png", folder / "approved.png", Tolerance{.levels = 1});
    REQUIRE_MESSAGE(approval.has_value(), (approval ? "" : approval.error().message));
    CHECK(approval->largestDifference == 1);
    CHECK(ErrorOf(CompareFiles(folder / "capture.png", folder / "approved.png", Tolerance{.levels = 0}))
              .starts_with("1 of 12 pixels is beyond the tolerance of 0 levels; the first is at (1, 0): "));
}

TEST_CASE("[0000-CA-08] visualproof refuses a file it cannot read, and says which one") {
    const std::filesystem::path folder = FreshFolder("unreadable");
    REQUIRE(nexum::platform::SavePng(MakeScene(), folder / "scene.png").has_value());
    {
        std::ofstream text(folder / "text.png");
        text << "this is not a PNG file";
    }

    const std::string missing =
        ErrorOf(CompareFiles(folder / "nothing.png", folder / "scene.png", Tolerance{.levels = 1}));
    CHECK(missing.starts_with("the capture cannot be read: "));
    CHECK(missing.find("nothing.png") != std::string::npos);

    const std::string notPng =
        ErrorOf(CompareFiles(folder / "scene.png", folder / "text.png", Tolerance{.levels = 1}));
    CHECK(notPng.starts_with("the approved image cannot be read: "));
    CHECK(notPng.find("text.png") != std::string::npos);
}
