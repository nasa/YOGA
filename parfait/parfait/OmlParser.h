#pragma once
#include <parfait/JsonParser.h>
#include <parfait/PomlParser.h>

namespace Parfait {
namespace OmlParser {
    inline Parfait::Dictionary parse(const std::string& s) {
        std::string json_error;
        std::string poml_error;
        try {
            return JsonParser::parse(s);
        } catch (const ParseException& json_exception) {
            json_error = json_exception.what();
        }

        try {
            return PomlParser::parse(s);
        } catch (const ParseException& poml_exception) {
            poml_error = poml_exception.what();
        }

        throw ParseException("Could not parse string as POML or JSON.\n" + json_error + "\n" + poml_error);
    }

}
}
