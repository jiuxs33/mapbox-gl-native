#pragma once

#include <mbgl/gl/headless_frontend.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/timer.hpp>

#include "metadata.hpp"

class TestRunner {
public:
    TestRunner(TestMetadata&& metadata_);
    double run();

private:
    void render();
    void runOperations();
    double checkImage();

    TestMetadata metadata;

    mbgl::util::RunLoop runloop;
    mbgl::HeadlessFrontend frontend;
    mbgl::Map map;

    mbgl::util::Timer timer;
    bool sleeping = false;

    mbgl::PremultipliedImage actual;
};