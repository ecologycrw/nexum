#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "Mu/App/ClientOptions.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Render/GpuBackend.hpp"

using mu::app::ClientOptions;
using mu::app::ParseClientOptions;
using nexum::render::GpuBackend;

namespace {

// The command line of MuClient with these options after the name of the program.
nexum::core::Result<ClientOptions> Parse(std::initializer_list<std::string_view> options) {
    std::vector<std::string_view> arguments{"MuClient"};
    arguments.insert(arguments.end(), options.begin(), options.end());
    return ParseClientOptions(arguments);
}

std::string ErrorOf(const nexum::core::Result<ClientOptions>& result) {
    REQUIRE_FALSE(result.has_value());
    return result.error().message;
}

} // namespace

TEST_CASE("without options, the client runs until the window closes, with the backend order of the system") {
    const auto options = Parse({});
    REQUIRE(options.has_value());
    CHECK_FALSE(options->showVersion);
    CHECK_FALSE(options->frameLimit.has_value());
    CHECK_FALSE(options->gpuBackend.has_value());
}

TEST_CASE("an empty command line, without even the name of the program, gives the same options") {
    const auto options = ParseClientOptions({});
    REQUIRE(options.has_value());
    CHECK_FALSE(options->frameLimit.has_value());
}

TEST_CASE("--version alone asks for the version") {
    const auto options = Parse({"--version"});
    REQUIRE(options.has_value());
    CHECK(options->showVersion);
}

TEST_CASE("--frames takes the number of frames, from 1 to 4294967295") {
    CHECK(Parse({"--frames", "300"})->frameLimit == 300U);
    CHECK(Parse({"--frames", "1"})->frameLimit == 1U);
    CHECK(Parse({"--frames", "4294967295"})->frameLimit == 4294967295U);
}

TEST_CASE("[0000-CA-06] --gpu-driver forces a backend, and an unknown name gives a clear error") {
    CHECK(Parse({"--gpu-driver", "vulkan"})->gpuBackend == GpuBackend::Vulkan);
    CHECK(Parse({"--gpu-driver", "direct3d12"})->gpuBackend == GpuBackend::Direct3D12);
    CHECK(Parse({"--gpu-driver", "metal"})->gpuBackend == GpuBackend::Metal);
    CHECK(ErrorOf(Parse({"--gpu-driver", "glide"})) ==
          "unknown GPU driver 'glide': use vulkan, direct3d12 or metal");
    CHECK(ErrorOf(Parse({"--gpu-driver", "Vulkan"})) ==
          "unknown GPU driver 'Vulkan': use vulkan, direct3d12 or metal");
}

TEST_CASE("the options may come in any order") {
    const auto options = Parse({"--gpu-driver", "direct3d12", "--frames", "5"});
    REQUIRE(options.has_value());
    CHECK(options->frameLimit == 5U);
    CHECK(options->gpuBackend == GpuBackend::Direct3D12);
}

TEST_CASE("--frames refuses anything but a whole number from 1 to 4294967295") {
    for (const std::string_view value :
         {"0", "-1", "+5", "abc", "12x", " 5", "5 ", "", "1.5", "4294967296", "99999999999999999999"}) {
        CAPTURE(value);
        CHECK(ErrorOf(Parse({"--frames", value})) ==
              "--frames needs a whole number from 1 to 4294967295, not '" + std::string(value) + "'");
    }
}

TEST_CASE("an option without its value is refused") {
    CHECK(ErrorOf(Parse({"--frames"})) == "--frames needs a value");
    CHECK(ErrorOf(Parse({"--frames", "3", "--gpu-driver"})) == "--gpu-driver needs a value");
}

TEST_CASE("an option given twice is refused") {
    CHECK(ErrorOf(Parse({"--frames", "3", "--frames", "4"})) == "--frames was given twice");
    CHECK(ErrorOf(Parse({"--gpu-driver", "vulkan", "--gpu-driver", "metal"})) ==
          "--gpu-driver was given twice");
}

TEST_CASE("unknown options are refused, and so is --version with other options") {
    CHECK(ErrorOf(Parse({"--unknown"})) == "unknown option '--unknown'");
    CHECK(ErrorOf(Parse({"--frames=300"})) == "unknown option '--frames=300'");
    CHECK(ErrorOf(Parse({"300"})) == "unknown option '300'");
    CHECK(ErrorOf(Parse({"--version", "--frames", "3"})) == "--version comes alone, without other options");
    CHECK(ErrorOf(Parse({"--version", "--version"})) == "--version comes alone, without other options");
}
