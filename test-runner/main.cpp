#include <mbgl/util/logging.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/filesystem.hpp>

#include "metadata.hpp"
#include "parser.hpp"
#include "runner.hpp"

#include <random>

int main(int argc, char** argv) {
    bool recycleMap;
    bool shuffle;
    uint32_t seedNumber;
    std::string testRootPath;
    std::vector<std::string> testNames;

    std::tie(recycleMap, shuffle, seedNumber, testRootPath, testNames) = parseArguments(argc, argv);

    const std::vector<std::string> ignores = parseIgnores();

    // Traverse all test root path if no test has been specified.
    if (testNames.empty()) {
        testNames.push_back({});
    }

    // Recursively traverse through the test paths and collect test directories containing "style.json".
    std::vector<mbgl::filesystem::path> testPaths;
    for (const auto& testName : testNames) {
        const auto absolutePath = mbgl::filesystem::path(testRootPath) / mbgl::filesystem::path(testName);
        for (auto& testPath : mbgl::filesystem::recursive_directory_iterator(absolutePath)) {
            if (testPath.path().filename() == "style.json") {
                testPaths.push_back(testPath);
            }
        }
    }

    if (shuffle) {
        std::seed_seq seed{ seedNumber };
        std::mt19937 shuffler(seed);
        std::shuffle(testPaths.begin(), testPaths.end(), shuffler);
    }

    for (auto& testPath : testPaths) {
        if (std::find(ignores.cbegin(), ignores.cend(), testPath.parent_path().string()) != ignores.end()) {
            mbgl::Log::Info(mbgl::Event::General, "Ignoring test %s", testPath.c_str());
            continue;
        }

        mbgl::optional<TestMetadata> metadata = TestMetadata::parseTestMetadata(testPath);
        if (metadata) {
            std::string testName = testPath.remove_filename().string();
            testName.erase(testName.find(testRootPath), testRootPath.length()+1);
            testName = testName.substr(0, testName.length()-1);

            mbgl::Log::Debug(mbgl::Event::General, "Running test %s", testName.c_str());
            const double allowed = metadata->allowed;

            TestRunner testMap(std::move(*metadata));
            double diff = testMap.run();
            bool result = diff <= allowed;
            mbgl::Log::Info(mbgl::Event::General, "Test %s [%lf <= %lf]: %s", testName.c_str(), diff, allowed, result ? "pass" : "fail");
        }
    }

    return 0;
}