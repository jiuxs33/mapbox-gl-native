#pragma once

#include <mbgl/util/filesystem.hpp>
#include <mbgl/util/rapidjson.hpp>

#include <tuple>
#include <string>
#include <vector>

struct TestMetadata;

using ArgumentsTuple = std::tuple<bool, bool, uint32_t, std::string, std::vector<std::string>>;

ArgumentsTuple parseArguments(int argc, char** argv);

std::vector<std::string> parseIgnores();

void localizeSourceURLs(mbgl::JSValue& root, mbgl::JSDocument& document);
void localizeStyleURLs(mbgl::JSValue& root, mbgl::JSDocument& document);