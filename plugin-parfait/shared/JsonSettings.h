#pragma once
#include <parfait/Dictionary.h>
#include <parfait/JsonParser.h>
#include <parfait/OmlParser.h>
#include <parfait/FileTools.h>

namespace PluginParfait {

inline bool isStringJson(std::string unknown_string) { return Parfait::StringTools::contains(unknown_string, ":"); }

inline Parfait::Dictionary parseJsonSettings(std::string filename_or_json) {
    Parfait::Dictionary json;

    if (not isStringJson(filename_or_json)) {
        json["grid_filename"] = filename_or_json;
    } else {
        json = Parfait::JsonParser::parse(filename_or_json);
    }
    if (Parfait::FileTools::doesFileExist("backdoor.poml")) {
        auto override_settings = Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString("backdoor.poml"));
        for (auto k : override_settings.keys()) {
            json[k] = override_settings[k];
        }
    }
    if (Parfait::FileTools::doesFileExist("backdoor.json")) {
        auto override_settings = Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString("backdoor.json"));
        for (auto k : override_settings.keys()) {
            json[k] = override_settings[k];
        }
    }

    return json;
}
}
