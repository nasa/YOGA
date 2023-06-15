#include <RingAssertions.h>
#include <utility>
#include <parfait/Dictionary.h>
#include <parfait/OmlParser.h>
#include <parfait/FileTools.h>
#include <parfait/VectorTools.h>
#include <parfait/DictionarySchemaValidator.h>

using namespace Parfait;

bool runValidatorFile(std::string file) {
    std::string folder = JSON_VALIDATOR_TESTS_DIR;

    auto json = Parfait::FileTools::loadFileToString(folder + "/" + file);

    auto dictionary = Parfait::JsonParser::parse(json);
    auto tests = dictionary.asObjects();
    bool all_validations_as_expected = true;
    for (auto test_case : tests) {
        DictionarySchema::Validator validator(test_case["schema"]);
        for (auto t : test_case["tests"].asObjects()) {
            auto status = validator.validate(t["data"]);
            if (t["valid"].asBool() != status.is_valid) {
                WARN("Should be " + Parfait::to_string(t["valid"].asBool()) + " was " +
                     Parfait::to_string(status.is_valid) + "\nFailed to correctly validate:\n" + t.dump(4) +
                     "\nSchema:\n" + validator.validation.dump(4) + "\nWith error message: \n" + status.message);
                all_validations_as_expected = false;
            }
        }
    }
    return all_validations_as_expected;
}

TEST_CASE("Run each json-schema-validator file", "[json-schema]") {
    REQUIRE(runValidatorFile("additionalProperties.json"));
    REQUIRE(runValidatorFile("type.json"));
}

TEST_CASE("additional properties with pattern match", "[json-schema]") {
    auto schema_string = R"({
"additionalProperties": false,
"patternProperties": {
    "^v": {}
},
"properties": {
    "bar": {},
    "foo": {}
}
})";
    auto data_string = R"({
"foo": 1,
"vroom": 2 }
)";
    DictionarySchema::Validator validator(JsonParser::parse(schema_string));
    auto dict = JsonParser::parse(data_string);
    REQUIRE(validator.validate(dict).is_valid);
}

TEST_CASE("additional properties explicitly allowed", "[json-schema-explicit]") {
    auto schema_string = R"({
"additionalProperties": { "type":"boolean"},
"properties": {
    "bar": {},
    "foo": {}
}
})";
    auto data_string = R"({ "foo": 1} )";
    DictionarySchema::Validator validator(JsonParser::parse(schema_string));
    auto dict = JsonParser::parse(data_string);
    REQUIRE(validator.validate(dict).is_valid);
}

TEST_CASE("Can validate a json object", "[json-schema]") {
    auto json = R"([{
"description": "The description of the test case",
"schema": {
    "description": "The schema against which the data in each test is validated",
    "type": "string"
},
"tests": [
    {
        "description": "Test for a valid instance",
        "data": "the instance to validate",
        "valid": true
    },
    {
        "description": "Test for an invalid instance",
        "data": 15,
        "valid": false
    }
]
}])";
    auto dictionary = Parfait::JsonParser::parse(json);
    auto tests = dictionary.asObjects();
    bool all_validations_as_expected = true;
    for (auto test_case : tests) {
        DictionarySchema::Validator validator(test_case["schema"]);
        for (auto t : test_case["tests"].asObjects()) {
            auto status = validator.validate(t["data"]);
            if (t["valid"].asBool() != status.is_valid) {
                WARN("Should be " + Parfait::to_string(t["valid"].asBool()) + " was " +
                     Parfait::to_string(status.is_valid) + "\nFailed to correctly validate:\ndata:" +
                     t["data"].dump(4) + "\nSchema:\n" + validator.validation.dump(4));
                all_validations_as_expected = false;
            }
        }
    }
    REQUIRE(all_validations_as_expected);
}

TEST_CASE("Regex matches") {
    std::string s = "vroom";
    std::string r = "^v";
    std::regex rx(r);
    bool b = std::regex_search(s, rx);
    REQUIRE(b);
}

TEST_CASE("Can set defaults from schema", "[json-schema]") {
    using namespace Parfait::json_literals;
    auto schema = R"({
"properties": {
    "pokemon": {
        "type": "string",
        "default": "Pikachu"
    },
    "pokedex": {
        "$ref": "#/definitions/Pokedex"
    }
},
"definitions": {
   "Pokedex": {
     "type": "object",
     "properties": {
        "model": {
          "type": "string",
          "default": "super-extra-new"
        },
        "capacity": {
          "type": "integer",
          "default": 3
        }
     }
   }
}
})"_json;
    Dictionary user_settings;
    auto options = DictionarySchema::mergeUserOptionsWithSchemaDefaults(schema, user_settings);
    REQUIRE("Pikachu" == options.at("pokemon").asString());
    REQUIRE("super-extra-new" == options.at("pokedex").at("model").asString());
    REQUIRE(3 == options.at("pokedex").at("capacity").asInt());
}

TEST_CASE("won't merge ", "[json-schema]") {
    using namespace Parfait::json_literals;
    auto schema = R"({
"properties":{
    "thermo":{
        "type":"object",
        "properties": {
            "viscosity": {
               "type":"number",
               "default":45
            },
            "Sutherland": {
                "type": "object",
                "oneOf":[
                   {
                      "type":"object",
                      "required":["mu0", "S0", "T0"],
                         "S0":{ "type":"number" },
                         "T0":{ "type":"number" },
                         "mu0":{ "type":"number" }
                   },
                   {
                       "type":"object",
                       "required":["S1", "S2"],
                           "S1":{ "type":"number" },
                           "T2":{ "type":"number" }
                   }
                ],
                "default": {
                    "S1":77,
                    "S2":79
                }
            }
        }
    }
}
})"_json;
    Parfait::Dictionary user_config;
    user_config["thermo"]["Sutherland"]["mu0"] = 88;
    user_config["thermo"]["Sutherland"]["T0"] = 89;
    user_config["thermo"]["Sutherland"]["S0"] = 90;

    auto full_config = DictionarySchema::mergeUserOptionsWithSchemaDefaults(schema, user_config);
    REQUIRE(full_config.has("thermo"));
    REQUIRE(full_config.at("thermo").has("viscosity"));
    REQUIRE(full_config.at("thermo").has("Sutherland"));
    REQUIRE(full_config.at("thermo").at("Sutherland").has("mu0"));
    REQUIRE(full_config["thermo"]["Sutherland"]["mu0"].asDouble() == 88);
    REQUIRE(full_config["thermo"]["Sutherland"]["T0"].asDouble() == 89);
    REQUIRE(full_config["thermo"]["Sutherland"]["S0"].asDouble() == 90);
}

// Todo:
// allOf.json
// anchor.json
// anyOf.json
// boolean_schema.json
// const.json
// contains.json
// content.json
// default.json
// defs.json
// dependentRequired.json
// dependentSchemas.json
// dog
// dynamicRef.json
// enum.json
// exclusiveMaximum.json
// exclusiveMinimum.json
// format.json
// id.json
// if-then-else.json
// infinite-loop-detection.json
// items.json
// maxContains.json
// maxItems.json
// maxLength.json
// maxProperties.json
// maximum.json
// minContains.json
// minItems.json
// minLength.json
// minProperties.json
// minimum.json
// multipleOf.json
// not.json
// oneOf.json
// optional
// pattern.json
// patternProperties.json
// prefixItems.json
// properties.json
// propertyNames.json
// ref.json
// refRemote.json
// required.json
// type.json
// unevaluatedItems.json
// unevaluatedProperties.json
// uniqueItems.json
// unknownKeyword.json