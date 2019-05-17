#include <mbgl/map/map_observer.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/style/image.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/logging.hpp>

#include <mapbox/pixelmatch.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "parser.hpp"
#include "runner.hpp"

#include <cassert>
#include <regex>

TestRunner::TestRunner(TestMetadata&& metadata_)
    : metadata(std::move(metadata_)),
      frontend(metadata.size, metadata.pixelRatio),
      map(frontend,
          mbgl::MapObserver::nullObserver(),
          mbgl::MapOptions()
              .withMapMode(metadata.mapMode)
              .withSize(metadata.size)
              .withPixelRatio(metadata.pixelRatio)
              .withCrossSourceCollisions(metadata.crossSourceCollisions),
          mbgl::ResourceOptions()) {
    map.setProjectionMode(mbgl::ProjectionMode()
                              .withAxonometric(metadata.axonometric)
                              .withXSkew(metadata.xSkew)
                              .withYSkew(metadata.ySkew));
}

double TestRunner::checkImage() {
    const std::string base = metadata.path.remove_filename().string();

#if !TEST_READ_ONLY
    if (getenv("UPDATE")) {
        mbgl::util::write_file(base + "/expected.png", mbgl::encodePNG(actual));
        return true;
    }
#endif

    mbgl::optional<std::string> maybeExpectedImage = mbgl::util::readFile(base + "/expected.png");
    if (!maybeExpectedImage) {
        mbgl::Log::Error(mbgl::Event::Setup, "Failed to load expected image %s", (base + "/expected.png").c_str());
        return false;
    }

    mbgl::PremultipliedImage expected = mbgl::decodeImage(*maybeExpectedImage);
    mbgl::PremultipliedImage diff { expected.size };

#if !TEST_READ_ONLY
    mbgl::util::write_file(base + "/actual.png", mbgl::encodePNG(actual));
#endif

    if (expected.size != actual.size) {
        mbgl::Log::Error(mbgl::Event::Setup, "Expected and actual image sizes differ");
        return false;
    }

    double pixels = mapbox::pixelmatch(actual.data.get(),
                                       expected.data.get(),
                                       expected.size.width,
                                       expected.size.height,
                                       diff.data.get(),
                                       0.1);

#if !TEST_READ_ONLY
    mbgl::util::write_file(base + "/diff.png", mbgl::encodePNG(diff));
#endif

    return pixels / (expected.size.width * expected.size.height);
}

void TestRunner::render() {
    runloop.runOnce();
    actual = frontend.render(map);
}

mbgl::filesystem::path localizeStylePath(const std::string& url) {
    static const std::regex localRegex("local://");
    static const mbgl::filesystem::path mvtFixturePath(std::string(TEST_RUNNER_ROOT_PATH) + "/vendor/mvt-fixtures/");
    static const mbgl::filesystem::path mbglStylesPath(std::string(TEST_RUNNER_ROOT_PATH) + "/vendor/");
    static const mbgl::filesystem::path integrationPath(std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/");

    mbgl::filesystem::path stylePath = std::regex_replace(url, localRegex, mvtFixturePath.string());
    if (mbgl::filesystem::exists(stylePath)) {
        return stylePath;
    }

    stylePath = std::regex_replace(url, localRegex, mbglStylesPath.string());
    if (mbgl::filesystem::exists(stylePath)) {
        return stylePath;
    }

    stylePath = std::regex_replace(url, localRegex, integrationPath.string());
    return stylePath;
}

void TestRunner::runOperations() {
    assert(metadata.document.HasMember("metadata"));
    assert(metadata.document["metadata"].HasMember("test"));
    assert(metadata.document["metadata"]["test"].HasMember("operations"));
    assert(metadata.document["metadata"]["test"]["operations"].IsArray());

    mbgl::JSValue& operationsValue = metadata.document["metadata"]["test"]["operations"];
    const auto& operationsArray = operationsValue.GetArray();
    if (operationsArray.Empty()) {
        return;
    }

    const auto& operationIt = operationsArray.Begin();
    assert(operationIt->IsArray());

    const auto& operationArray = operationIt->GetArray();
    assert(operationArray.Size() >= 1u);

    // TODO: operations
    //  - wait
    //  - sleep
    //  - addImage
    //  - updateImage
    //  - setStyle

    //  - setCenter
    //  - setZoom
    //  - setBearing
    //  - setPitch
    //  - setLight
    static const char* waitOp = "wait";
    static const char* sleepOp = "sleep";
    static const char* addImageOp = "addImage";
    static const char* updateImageOp = "updateImage";
    static const char* setStyleOp = "setStyle";

    // wait
    if (strcmp(operationArray[0].GetString(), waitOp) == 0) {
        render();

    // sleep
    } else if (strcmp(operationArray[0].GetString(), sleepOp) == 0) {
        if (sleeping) {
            sleeping = false;
        } else {
            mbgl::Duration duration = mbgl::Seconds(20);
            if (operationArray.Size() == 2u) {
                duration = mbgl::Milliseconds(std::atoi(operationArray[1].GetString()));
            }
            sleeping = true;
            timer.start(duration, mbgl::Duration::zero(), [&]() { runOperations(); });
            return;
        }

    // addImage | updateImage
    } else if (strcmp(operationArray[0].GetString(), addImageOp) == 0 || strcmp(operationArray[0].GetString(), updateImageOp) == 0) {
        assert(operationArray.Size() >= 3u);

        // TODO: pixelRatio
        float pixelRatio = metadata.pixelRatio;

        std::string imageName = operationArray[1].GetString();
        imageName.erase(0, 1);
        imageName.erase(imageName.size() - 1);

        std::string imagePath = operationArray[2].GetString();
        imagePath.erase(0, 1);
        imagePath.erase(imagePath.size() - 1);

        static const mbgl::filesystem::path filePath(std::string(TEST_RUNNER_ROOT_PATH) + "/mapbox-gl-js/test/integration/" + imagePath);

        mbgl::optional<std::string> maybeImage = mbgl::util::readFile(filePath.string());
        if (!maybeImage) {
            mbgl::Log::Error(mbgl::Event::Setup, "Failed to load expected image %s",
                             filePath.c_str());
            return;
        }

        printf("%s\n", filePath.c_str());
        map.getStyle().addImage(std::make_unique<mbgl::style::Image>(imageName, mbgl::decodeImage(*maybeImage), pixelRatio));
        imageName.erase(imageName.size() - 1);
        render();

    // setStyle
    } else if (strcmp(operationArray[0].GetString(), setStyleOp) == 0) {
        assert(operationArray.Size() == 2u);
        if (operationArray[1].IsString()) {
            auto maybeJSON = mbgl::util::readFile(localizeStylePath(operationArray[1].GetString()).string());
            if (!maybeJSON) {
                mbgl::Log::Error(mbgl::Event::General, "Unable to open style file %s", operationArray[1].GetString());
                return;
            }

            mbgl::JSDocument document;
            document.Parse<0>(*maybeJSON);
            if (document.HasParseError()) {
                mbgl::Log::Error(mbgl::Event::ParseStyle, mbgl::formatJSONParseError(document).c_str());
                return;
            }

            localizeStyleURLs(document, document);

            rapidjson::StringBuffer buffer;
            buffer.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);

            std::string styleJson(buffer.GetString());
            printf("style:\n%s\n", styleJson.c_str());
            map.getStyle().loadJSON(styleJson);
            render();
        } else {
            localizeStyleURLs(operationArray[1], metadata.document);

            rapidjson::StringBuffer buffer;
            buffer.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            operationArray[1].Accept(writer);

            std::string styleJson(buffer.GetString());
            printf("style:\n%s\n", styleJson.c_str());
            map.getStyle().loadJSON(styleJson);
            render();
        }
    } else {
        mbgl::Log::Debug(mbgl::Event::Setup, "Executing operation %s", operationArray[0].GetString());
        exit(1);
    }

    operationsValue.Erase(operationsValue.Begin());
}

double TestRunner::run() {

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    metadata.document.Accept(writer);

    std::string styleJson(buffer.GetString());

    printf("style:\n%s\n", styleJson.c_str());

    map.getStyle().loadJSON(styleJson);

    render();

    if (metadata.hasOperations) {
        runOperations();
    }

    return checkImage();
}