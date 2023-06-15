#include <RingAssertions.h>
#include <utility>
#include <parfait/Dictionary.h>
#include <parfait/DictionaryRules.h>
#include <parfait/JsonParser.h>

using namespace Parfait;
using namespace Parfait::json_literals;
using namespace Parfait::StringTools;

TEST_CASE("Required keys") {
    auto r = Rules().requireField("dog");
    REQUIRE(r.check(R"({"dog":1})"_json));
    auto s = r.check("{}"_json);
    REQUIRE_FALSE(s);
    REQUIRE(contains(s.message, "missing required key: dog"));
}

TEST_CASE("type check") {
    Rules r;
    r["dog"].requireType(Rules::string);
    REQUIRE(r.check(R"({"dog":"tayla"})"_json));
    auto s = r.check(R"({"dog":false})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("type must be string but is boolean"));
}

TEST_CASE("Don't allow additional properties") {
    SECTION("Allow any additional properties by default") { REQUIRE(Rules().check(R"({"dog":"tayla"})"_json)); }
    SECTION("Can disable additional properties") {
        auto r = Rules().disallowAdditionalFields();
        auto s = r.check(R"({"dog":"tayla"})"_json);
        REQUIRE_FALSE(s);
        REQUIRE_THAT(s.message, Contains("disallowed additional property found: dog"));
    }
    SECTION("Allow required properties") {
        auto rules = Rules().requireField("dog");
        REQUIRE(rules.check(R"({"dog":"tayla"})"_json));
    }
    SECTION("Allow not required properties") {
        auto rules = Rules().requireField("dog").disallowAdditionalFields();
        rules["cat"].requireType("string");
        REQUIRE(rules.check(R"({"dog":"tayla", "cat":"lol no"})"_json));
    }
}

TEST_CASE("Object can satisfy oneOf a potential list of rules") {
    auto type1 = Rules().requireType(Rules::number);
    auto type2 = Rules().requireType(Rules::string);

    auto r = Rules();
    r["dog"].addOneOf(type1).addOneOf(type2);
    REQUIRE(r.check(R"({"dog":1})"_json));
    REQUIRE(r.check(R"({"dog":"brown"})"_json));

    auto s = r.check(R"({"dog":true})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s, Contains("not one of"));
    s = r.check(R"({"dog":[1,2,3]})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s, Contains("not one of"));
}

TEST_CASE("Can required options (enums)") {
    Rules r;
    r["pet"].addOption("dog").addOption("cat");
    REQUIRE(r.check(R"({"pet":"dog"})"_json));
    REQUIRE(r.check(R"({"pet":"cat"})"_json));
    // bird people...
    auto s = r.check(R"({"pet":"bird"})"_json);
    REQUIRE_FALSE(s);
    REQUIRE((StringTools::contains(s.message, "bird is not a valid option:") and
             StringTools::contains(s.message, "dog") and StringTools::contains(s.message, "cat")));
}

TEST_CASE("Can require within min or max bounds") {
    Rules r;
    r["pressure"].requireType(Rules::number).requireMinimum(0.0).requireMaximum(2e7);
    REQUIRE(r.check(R"({"pressure":1.4})"_json));
    auto s = r.check(R"({"pressure":-1.2})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("is below minimum value"));

    s = r.check(R"({"pressure":2e8})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("is above maximum value"));
}

TEST_CASE("Can require arrays have min or max length") {
    Rules r;
    r["mass fractions"].requireType(Rules::array).requireMinimumLength(1).requireMaximumLength(3);
    REQUIRE(r.check(R"({"mass fractions":[1.4]})"_json));
    auto s = r.check(R"({"mass fractions":[]})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("is below minimum length"));

    s = r.check(R"({"mass fractions":[1,2,3,4]})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("is above maximum length"));
}

TEST_CASE("Can require a boolean is true (or false)") {
    Rules r;
    r["do bad things"].requireType(Rules::boolean).requireFalse();

    auto s = r.check(R"({"do bad things":false})"_json);
    REQUIRE(s);
    s = r.check(R"({"do bad things":true})"_json);
    REQUIRE_FALSE(s);
}

TEST_CASE("Can require a string value") {
    Rules r;
    r["do bad things"].requireEquals("yeah boy!");

    auto s = r.check(R"({"do bad things":"yeah boy!"})"_json);
    printf("s.message %s\n", s.message.c_str());
    REQUIRE(s);
    s = r.check(R"({"do bad things":"boooooring"})"_json);
    REQUIRE_FALSE(s);
}

TEST_CASE("Can have custom error messages if a rule breaks") {
    Rules r;
    r["slope limiter"]
        .addOption("first order")
        .addOption("no limiter")
        .addCustomHelpMessage("We only support all or nothing!");
    auto s = r.check(R"({"slope limiter":"Barth"})"_json);
    REQUIRE_FALSE(s);
    REQUIRE_THAT(s.message, Contains("We only support all or nothing!"));
}