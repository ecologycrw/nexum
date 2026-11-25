#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "Nexum/Core/Result.hpp"
#include "VisualProof/ProofOptions.hpp"

using visualproof::ParseProofOptions;
using visualproof::ProofOptions;

namespace {

// The command line of visualproof with these arguments after the name of the program.
nexum::core::Result<ProofOptions> Parse(std::initializer_list<std::string_view> given) {
    std::vector<std::string_view> arguments{"visualproof"};
    arguments.insert(arguments.end(), given.begin(), given.end());
    return ParseProofOptions(arguments);
}

std::string ErrorOf(const nexum::core::Result<ProofOptions>& result) {
    REQUIRE_FALSE(result.has_value());
    return result.error().message;
}

} // namespace

TEST_CASE("visualproof takes the capture, the approved image and the tolerance of the scene") {
    const auto options = Parse({"capture.png", "approved.png", "--tolerance", "1"});
    REQUIRE(options.has_value());
    CHECK(options->capture == "capture.png");
    CHECK(options->approved == "approved.png");
    CHECK(options->tolerance.levels == 1);
}

TEST_CASE("the tolerance may come anywhere, and goes from 0 to 255 levels") {
    CHECK(Parse({"--tolerance", "0", "a.png", "b.png"})->tolerance.levels == 0);
    CHECK(Parse({"a.png", "--tolerance", "255", "b.png"})->tolerance.levels == 255);
    CHECK(Parse({"a.png", "--tolerance", "17", "b.png"})->approved == "b.png");
}

TEST_CASE("visualproof refuses a tolerance that is not a whole number from 0 to 255") {
    for (const std::string_view value : {"256", "-1", "+1", "1.5", "one", "", " 1", "1 "}) {
        CAPTURE(value);
        CHECK(ErrorOf(Parse({"a.png", "b.png", "--tolerance", value})) ==
              "--tolerance needs a whole number from 0 to 255, not '" + std::string(value) + "'");
    }
}

TEST_CASE("each scene states its tolerance: visualproof has no default") {
    CHECK(ErrorOf(Parse({"a.png", "b.png"})) ==
          "--tolerance is missing: each scene states its own tolerance (docs/VISUAL.md)");
    CHECK(ErrorOf(Parse({"a.png", "b.png", "--tolerance"})) == "--tolerance needs a value");
    CHECK(ErrorOf(Parse({"a.png", "b.png", "--tolerance", "1", "--tolerance", "2"})) ==
          "--tolerance was given twice");
}

TEST_CASE("visualproof needs exactly two images, and refuses unknown options") {
    CHECK(ErrorOf(Parse({})) == "visualproof needs the capture and the approved image");
    CHECK(ErrorOf(Parse({"a.png", "--tolerance", "1"})) ==
          "visualproof needs the capture and the approved image");
    CHECK(ErrorOf(Parse({"a.png", "b.png", "c.png", "--tolerance", "1"})) == "unexpected argument 'c.png'");
    CHECK(ErrorOf(Parse({"a.png", "b.png", "--tolerance=1"})) == "unknown option '--tolerance=1'");
    CHECK(ErrorOf(Parse({"a.png", "b.png", "--tolerance", "1", "-v"})) == "unknown option '-v'");
}
