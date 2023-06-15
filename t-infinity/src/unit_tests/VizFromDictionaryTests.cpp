#include <RingAssertions.h>
#include <parfait/JsonParser.h>
#include <parfait/Checkpoint.h>
#include <t-infinity/VizFromDictionary.h>

TEST_CASE("viz from dictionary can be a line") {
    using namespace Parfait::json_literals;

    SECTION("Valid line") {
        auto dict = R"({"type":"line","a":[0,0,0], "b":[1,0,0]})"_json;
        auto rules = inf::getVisualizeFromDictionaryRules();
        REQUIRE(rules.check(dict));
    }

    SECTION("line requires points a and b") {
        auto dict = R"({"type":"line"})"_json;
        auto rules = inf::getVisualizeFromDictionaryRules();
        {
            auto m = rules.check(dict);
            REQUIRE_FALSE(m);
            REQUIRE(Parfait::StringTools::contains(m.message, "missing required key: a"));
        }
        {
            dict["a"] = 0.0;
            auto m = rules.check(dict);
            REQUIRE_FALSE(m);
            REQUIRE(Parfait::StringTools::contains(m.message, "missing required key: b"));
        }
    }
}
TEST_CASE("viz from dictionary tags sampling") {
    Parfait::Dictionary surface_sample;
    surface_sample["surface tags"] = "no";
    surface_sample["type"] = "plane";
    surface_sample["center"] = std::vector<double>{0, 0, 0.5};
    surface_sample["normal"] = std::vector<double>{0, 0, 1};
    auto rules = inf::getVisualizeFromDictionaryRules();
    auto status = rules.check(surface_sample);
    CHECK(status);
}
