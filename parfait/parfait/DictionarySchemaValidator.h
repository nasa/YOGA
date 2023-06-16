#pragma once
#include <queue>
#include <regex>
#include "Dictionary.h"
#include "VectorTools.h"
#include "StringTools.h"
#include "ToString.h"

namespace Parfait {
namespace DictionarySchema {
    struct Status {
        bool is_valid;
        std::string message;
    };

    class Test {
      public:
        virtual void test(const Dictionary& dict, Status& s) const = 0;
        virtual std::string name() const = 0;
        virtual ~Test() = default;
    };

    class AdditionalPropertiesTest : public Test {
      public:
        inline AdditionalPropertiesTest(const Dictionary& v) : validation(v) {}
        inline std::string name() const override { return "additionalProperties"; }
        inline void test(const Parfait::Dictionary& d, Status& status) const override {
            try {
                std::string additional = "additionalProperties";
                if (not validation.has(additional)) return;
                if (validation.at(additional).type() == DictionaryEntry::Boolean and
                    validation.at(additional).asBool() == true)
                    return;
                std::vector<std::string> explicitly_allowed_keys;
                if (validation.has("properties")) explicitly_allowed_keys = validation.at("properties").keys();
                auto dict_keys = d.keys();

                std::vector<std::string> remaining_invalid_keys;
                for (const auto& k : dict_keys) {
                    if (not Parfait::VectorTools::isIn(explicitly_allowed_keys, k)) {
                        remaining_invalid_keys.push_back(k);
                    }
                }

                std::string pattern_key = "patternProperties";
                if (validation.has(pattern_key)) {
                    auto patterns_to_match = validation.at(pattern_key).keys();
                    if (patterns_to_match.size() > 1) {
                        PARFAIT_THROW("Expected only one regex pattern to match.  Guess you have to check against any");
                    }
                    auto pattern_to_match = patterns_to_match.front();
                    auto current_remaining_invalid_keys = remaining_invalid_keys;
                    remaining_invalid_keys.clear();
                    for (const auto& key : current_remaining_invalid_keys) {
                        if (not doAllStringsMatchPattern(pattern_to_match, {key}))
                            remaining_invalid_keys.push_back(key);
                    }
                }

                if (validation.at(additional).type() == DictionaryEntry::Object and
                    validation.at(additional).has("type")) {
                    auto required_type = validation.at(additional).at("type").asString();
                    auto current_remaining_invalid_keys = remaining_invalid_keys;
                    remaining_invalid_keys.clear();
                    for (const auto& key : current_remaining_invalid_keys) {
                        if (d.at(key).typeString() != required_type) remaining_invalid_keys.push_back(key);
                    }
                }

                if (remaining_invalid_keys.empty()) {
                    status.is_valid = true;
                    return;
                }
                status.message += "Additional properties found!\nProperties: " + Parfait::to_string(dict_keys) +
                                  "\nExpected:" + Parfait::to_string(explicitly_allowed_keys) + "\n";
                status.is_valid = false;
            } catch (std::exception& e) {
                status.is_valid = false;
                status.message += "Exception: " + std::string(e.what());
                return;
            }
        }

      private:
        const Dictionary& validation;
        inline bool doAllStringsMatchPattern(std::string pattern,
                                             const std::vector<std::string>& strings_that_must_match) const {
            std::regex r(pattern);
            for (const auto& s : strings_that_must_match) {
                if (not std::regex_search(s, r)) return false;
            }

            return true;
        }
    };

    class TypeTest : public Test {
      public:
        inline TypeTest(const Dictionary& validation) : validation(validation) {}
        inline std::string name() const override { return "type"; }

        inline void testIsSingleType(const Dictionary& dict, std::string type, Status& status) const {
            if (type == "number") {  // numbers can be double or int
                if (dict.type() == DictionaryEntry::Integer or dict.type() == DictionaryEntry::Double) {
                    status.is_valid = true;
                    return;
                }
            }
            if (type == "integer" and dict.type() == Dictionary::Double) {
                double d = dict.asDouble();
                int i = round(d);
                if (double(i) == d) {
                    status.is_valid = true;
                    return;
                }
            }

            std::vector<Dictionary::TYPE> array_types = {
                Dictionary::ObjectArray, Dictionary::IntArray, Dictionary::BoolArray, Dictionary::DoubleArray};
            if (type == "array" and
                std::any_of(array_types.begin(), array_types.end(), [&](auto t) { return t == dict.type(); })) {
                status.is_valid = true;
                return;
            }

            if (dict.type() != getType(type)) {
                status.is_valid = false;
                status.message += "Type should be " + type + " but was " + dict.typeString();
            } else {
                status.is_valid = true;
            }
        }

        inline void test(const Dictionary& dict, Status& status) const override {
            if (not validation.has("type")) return;
            std::vector<std::string> types;
            if (validation.at("type").isArrayObject())
                types = validation.at("type").asStrings();
            else
                types.push_back(validation.at("type").asString());
            status.is_valid = false;
            for (auto type : types) {
                Status single_status;
                testIsSingleType(dict, type, single_status);
                if (single_status.is_valid) {
                    status.message = "";
                    status.is_valid = true;
                    return;
                }
            }
        }

      private:
        const Dictionary& validation;
        inline Dictionary::TYPE getType(std::string type) const {
            if (type == "string") {
                return Dictionary::String;
            }
            if (type == "integer") {
                return Dictionary::Integer;
            }
            if (type == "number") {
                return Dictionary::Double;
            }
            if (type == "object") {
                return Dictionary::Object;
            }
            if (type == "array") {
                return Dictionary::ObjectArray;
            }
            if (type == "boolean") {
                return Dictionary::Boolean;
            }
            if (type == "null") {
                return Dictionary::Null;
            }
            PARFAIT_THROW("Unknown Dictionary Validation type: " + type);
        }
    };

    class Validator {
      public:
        inline Validator(Parfait::Dictionary schema) : validation(std::move(schema)) {
            tests.emplace_back(std::make_unique<TypeTest>(validation));
            tests.emplace_back(std::make_unique<AdditionalPropertiesTest>(validation));
        }
        inline Status validate(const Parfait::Dictionary& dict) {
            Status status;
            status.is_valid = true;
            for (const auto& t : tests) {
                try {
                    t->test(dict, status);
                } catch (std::exception& e) {
                    status.is_valid = false;
                    status.message += e.what();
                }
                if (not status.is_valid) {
                    return status;
                }
            }
            return status;
        }

      public:
        Dictionary validation;
        std::vector<std::unique_ptr<Test>> tests;
    };

    inline Dictionary extractRef(const Dictionary& global_schema, std::string reference_string) {
        auto q = StringTools::split(reference_string, "/");
        Dictionary ref = global_schema;
        for (auto key : q) {
            if (key == "#") {
                continue;
            } else {
                Dictionary after = ref.at(key);
                ref = after;
            }
        }
        return ref;
    }

    inline Dictionary mergeUserOptionsWithSchemaDefaults(const Dictionary& global_schema,
                                                         const Dictionary& schema,
                                                         Dictionary user_options) {
        if (not schema.has("properties")) return user_options;
        for (const auto& property : schema.at("properties").keys()) {
            auto& subschema = schema.at("properties").at(property);
            if (subschema.has("default")) {
                if (not user_options.has(property)) user_options[property] = subschema.at("default");
            } else if (subschema.has("properties")) {
                if (not user_options.has(property)) user_options[property] = false;
                user_options[property] =
                    mergeUserOptionsWithSchemaDefaults(global_schema, subschema, user_options[property]);
            } else if (subschema.has("$ref")) {
                auto ref = extractRef(global_schema, subschema.at("$ref").asString());
                user_options[property] = mergeUserOptionsWithSchemaDefaults(global_schema, ref, user_options[property]);
            }
        }
        return user_options;
    }

    inline Dictionary mergeUserOptionsWithSchemaDefaults(const Dictionary& global_schema,
                                                         Dictionary global_user_options) {
        return mergeUserOptionsWithSchemaDefaults(global_schema, global_schema, global_user_options);
    }

}
}
