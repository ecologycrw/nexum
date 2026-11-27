// The main function of a fuzz target built as a normal test (cmake/NexumFuzzing.cmake): it runs the fuzz
// function once with no bytes and once on each file of the folder of seeds, in the order of the names. A
// crash, a failed check of a sanitizer or a folder without seeds fails the test.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

namespace {

[[nodiscard]] std::vector<std::filesystem::path> Seeds(const std::filesystem::path& folder) {
    std::vector<std::filesystem::path> seeds;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (entry.is_regular_file()) {
            seeds.push_back(entry.path());
        }
    }
    std::sort(seeds.begin(), seeds.end());
    return seeds;
}

[[nodiscard]] std::vector<std::uint8_t> ReadAll(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    const std::vector<char> text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::vector<std::uint8_t> bytes(text.size());
    std::transform(text.begin(), text.end(), bytes.begin(),
                   [](char letter) { return static_cast<std::uint8_t>(letter); });
    return bytes;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fputs("usage: <fuzz target>_seeds <folder of seeds>\n", stderr);
        return 2;
    }
    const std::filesystem::path folder(argv[1]);
    const auto seeds = Seeds(folder);
    if (seeds.empty()) {
        std::fprintf(stderr, "no seeds in the folder %s\n", folder.string().c_str());
        return 1;
    }
    static_cast<void>(LLVMFuzzerTestOneInput(nullptr, 0));
    for (const auto& seed : seeds) {
        const std::vector<std::uint8_t> bytes = ReadAll(seed);
        static_cast<void>(LLVMFuzzerTestOneInput(bytes.data(), bytes.size()));
    }
    std::printf("ran %zu seeds of %s\n", seeds.size(), folder.string().c_str());
    return 0;
}
