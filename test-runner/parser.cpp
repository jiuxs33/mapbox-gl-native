#include <mbgl/util/logging.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/rapidjson.hpp>

#include <args.hxx>

#include "parser.hpp"
#include "metadata.hpp"

#include <regex>

ArgumentsTuple parseArguments(int argc, char** argv) {
    args::ArgumentParser argumentParser("Mapbox GL Test Runner");

    args::HelpFlag helpFlag(argumentParser, "help", "Display this help menu", { 'h', "help" });

    args::Flag recycleMapFlag(argumentParser, "recycle map", "Toggle reusing the map object",
                              { 'r', "recycle-map" });
    args::Flag shuffleFlag(argumentParser, "shuffle", "Toggle shuffling the tests order",
                           { 's', "shuffle" });
    args::ValueFlag<uint32_t> seedValue(argumentParser, "seed", "Shuffle seed (default: random)",
                                        { "seed" });
    args::ValueFlag<std::string> testPathValue(argumentParser, "rootPath", "Test root rootPath",
                                               { 'p', "rootPath" });
    args::PositionalList<std::string> testNameValues(argumentParser, "URL", "Test name(s)");

    try {
        argumentParser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::ostringstream stream;
        stream << argumentParser;
        mbgl::Log::Info(mbgl::Event::General, stream.str());
        exit(0);
    } catch (const args::ParseError& e) {
        std::ostringstream stream;
        stream << argumentParser;
        mbgl::Log::Info(mbgl::Event::General, stream.str());
        mbgl::Log::Error(mbgl::Event::General, e.what());
        exit(1);
    } catch (const args::ValidationError& e) {
        std::ostringstream stream;
        stream << argumentParser;
        mbgl::Log::Info(mbgl::Event::General, stream.str());
        mbgl::Log::Error(mbgl::Event::General, e.what());
        exit(2);
    }

    const std::string testDefaultPath =
        std::string(TEST_RUNNER_ROOT_PATH).append("/mapbox-gl-js/test/integration/render-tests");

    return ArgumentsTuple {
        recycleMapFlag ? args::get(recycleMapFlag) : false,
        shuffleFlag ? args::get(shuffleFlag) : false, seedValue ? args::get(seedValue) : 1u,
        testPathValue ? args::get(testPathValue) : testDefaultPath, args::get(testNameValues)
    };
}

std::vector<std::string> parseIgnores() {
    std::vector<std::string> ignores;

    mbgl::filesystem::path path =
        mbgl::filesystem::path(TEST_RUNNER_ROOT_PATH).append("platform/node/test/ignores.json");

    auto maybeIgnores = mbgl::util::readFile(path.string());
    if (!maybeIgnores) {
        mbgl::Log::Error(mbgl::Event::ParseStyle, "Unable to open ignores file %s",
                         path.c_str());
        return ignores;
    }

    mbgl::JSDocument doc;
    doc.Parse<0>(*maybeIgnores);
    if (doc.HasParseError()) {
        mbgl::Log::Error(mbgl::Event::ParseStyle, mbgl::formatJSONParseError(doc).c_str());
        return ignores;
    }

    for (const auto& property : doc.GetObject()) {
        const std::string ignore = { property.name.GetString(), property.name.GetStringLength() };
        auto ignorePath =
            mbgl::filesystem::path(TEST_RUNNER_ROOT_PATH).append("mapbox-gl-js/test/integration").append(ignore);
        ignores.push_back(ignorePath.string());
    }

    return ignores;
}

std::string localizeLocalURL(const std::string& url) {
    static const std::regex regex("local://");
    static const mbgl::filesystem::path filePath("file://" + std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/");
    return std::regex_replace(url, regex, filePath.string());
}

std::string localizeMapboxSpriteURL(const std::string& url) {
    static const std::regex regex("mapbox://");
    static const mbgl::filesystem::path filePath("file://" + std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/");
    return std::regex_replace(url, regex, filePath.string());
}

std::string localizeMapboxFontsURL(const std::string& url) {
    static const std::regex regex("mapbox://fonts");
    static const mbgl::filesystem::path filePath("file://" + std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/glyphs");
    return std::regex_replace(url, regex, filePath.string());
}

std::string localizeMapboxTilesURL(const std::string& url) {
    static const std::regex regex("mapbox://");
    static const mbgl::filesystem::path filePath("file://" + std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/tiles/");
    return std::regex_replace(url, regex, filePath.string());
}

std::string localizeMapboxTilesetURL(const std::string& url) {
    static const std::regex regex("mapbox://");
    static const mbgl::filesystem::path filePath("file://" + std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/tilesets/");
    return std::regex_replace(url, regex, filePath.string());
}

void localizeSourceURLs(mbgl::JSValue& root, mbgl::JSDocument& document) {
    for (auto& sourceProperty : root.GetObject()) {
        mbgl::JSValue& sourceValue = sourceProperty.value;
        assert(sourceValue.HasMember("type"));
        const std::string& typeValue = sourceValue["type"].GetString();
        if (typeValue == "vector" || typeValue == "raster" || typeValue == "raster-dem") {
            if (sourceValue.HasMember("url")) {
                mbgl::JSValue& urlValue = sourceValue["url"];
                urlValue.Set<std::string>(localizeLocalURL(urlValue.GetString()),
                                          document.GetAllocator());
            } else if (sourceValue.HasMember("tiles")) {
                mbgl::JSValue& tilesValue = sourceValue["tiles"];
                for (auto& tileValue : tilesValue.GetArray()) {
                    tileValue.Set<std::string>(localizeLocalURL(tileValue.GetString()),
                                               document.GetAllocator());
                }
            }
        } else if (typeValue == "image") {
            if (sourceValue.HasMember("url")) {
                mbgl::JSValue& urlValue = sourceValue["url"];
                urlValue.Set<std::string>(localizeLocalURL(urlValue.GetString()),
                                          document.GetAllocator());
            }
        } else if (sourceValue.HasMember("video")) {
            mbgl::JSValue& urlsValue = sourceValue["urls"];
            for (auto& urlValue : urlsValue.GetArray()) {
                urlValue.Set<std::string>(localizeLocalURL(urlValue.GetString()),
                                          document.GetAllocator());
            }
        } else if (typeValue == "geojson") {
            if (sourceValue.HasMember("data") && sourceValue["data"].IsString()) {
                mbgl::JSValue& dataValue = sourceValue["data"];
                dataValue.Set<std::string>(localizeLocalURL(dataValue.GetString()),
                                           document.GetAllocator());
            }
        }
    }
}

void localizeStyleURLs(mbgl::JSValue& root, mbgl::JSDocument& document) {
    if (root.HasMember("sources")) {
        localizeSourceURLs(root["sources"], document);
    }

    if (root.HasMember("glyphs")) {
        mbgl::JSValue& glyphsValue = root["glyphs"];
        glyphsValue.Set<std::string>(localizeLocalURL(glyphsValue.GetString()), document.GetAllocator());
    }

    if (root.HasMember("sprite")) {
        mbgl::JSValue& spriteValue = root["sprite"];
        spriteValue.Set<std::string>(localizeLocalURL(spriteValue.GetString()), document.GetAllocator());
    }
}