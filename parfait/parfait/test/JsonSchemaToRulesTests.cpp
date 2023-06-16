#include <RingAssertions.h>
#include <parfait/JsonParser.h>
#include <parfait/DictionaryRules.h>
#include "parfait/Checkpoint.h"

Parfait::Rules makeFromSchema(const Parfait::Dictionary& schema) {
    Parfait::Rules r;
    if (schema.has("type")) {
        r.requireType(Parfait::Rules::TypeFromString(schema.at("type").asString()));
    }
    if (schema.has("properties")) {
        for (auto key : schema.at("properties").keys()) {
            auto p = schema.at("properties").at(key);
            r[key] = makeFromSchema(schema.at("properties").at(key));
        }
    }
    if (schema.has("minimum")) {
        r.requireMinimum(schema.at("minimum").asDouble());
    }
    if (schema.has("maximum")) {
        r.requireMaximum(schema.at("maximum").asDouble());
    }
    std::string additional_properties = "additionalProperties";

    if (schema.has(additional_properties)) {
        if (schema.at(additional_properties).type() == Parfait::Dictionary::TYPE::Boolean) {
            if (not schema.at(additional_properties).asBool()) {
                r.disallowAdditionalFields();
                printf("disallowing additional properties");
            }
        }
    }
    return r;
}

TEST_CASE("Can translate json schema to Parfait::Rules") {
    using namespace Parfait::json_literals;
    auto test_schema = R"(
{"type": "object",
  "properties": {
    "firstName": {
      "type": "string",
      "description": "The person's first name."
    },
    "lastName": {
      "type": "string",
      "description": "The person's last name."
    },
    "age": {
      "description": "Age in years which must be equal to or greater than zero.",
      "type": "integer",
      "minimum": 0
    }
  },
  "additionalProperties":false
})"_json;

    REQUIRE(test_schema.has("type"));

    Parfait::Rules r = makeFromSchema(test_schema);
    Parfait::Dictionary dict;
    dict["dog"] = 1;
    auto s = r.check(dict.at("dog"));
    REQUIRE_THAT(s.message, Contains("type must be object"));
    REQUIRE_FALSE(s.value);

    dict["dog"] = R"({"firstName":1})"_json;
    s = r.check(dict.at("dog"));
    REQUIRE_THAT(s.message, Contains("type must be string but is integer"));
    REQUIRE_FALSE(s);

    dict["dog"] = R"({"age":-1})"_json;
    s = r.check(dict.at("dog"));
    REQUIRE_THAT(s.message, Contains("is below minimum value 0"));
    REQUIRE_FALSE(s);

    dict["dog"] = R"({"age":1, "purple":"not allowed"})"_json;
    s = r.check(dict.at("dog"));
    REQUIRE_THAT(s.message, Contains("disallowed additional property found: purple"));
    REQUIRE_FALSE(s);
}

// Todo:
//  - enum
//  - additionalProperties maps to object?
//  - required
//  - min/max length