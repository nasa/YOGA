#include <RingAssertions.h>
#include <parfait/PomlParser.h>

using namespace Parfait;

TEST_CASE("turn toml into dictionaries") {
    SECTION("an empty string is an empty dict") {
        std::string s;
        auto dict = PomlParser::parse(s);
        REQUIRE(0 == dict.size());
    }

    SECTION("Key value pairs") {
        std::string s = "title = \"TOML Example\"\n";
        s += "pokemon = 9001\n";
        s += "has_wings = false\n";
        s += "mass = 98.5\n";
        s += "\"quoted keys are ok\" = \"dog\"\n";
        auto dict = PomlParser::parse(s);
        REQUIRE(1 == dict.count("title"));
        REQUIRE("TOML Example" == dict["title"].asString());
        REQUIRE(1 == dict.count("pokemon"));
        REQUIRE(9001 == dict["pokemon"].asInt());
        REQUIRE(1 == dict.count("has_wings"));
        REQUIRE_FALSE(dict["has_wings"].asBool());
        REQUIRE(1 == dict.count("mass"));
        REQUIRE(98.5 == dict["mass"].asDouble());
        REQUIRE(dict.has("quoted keys are ok"));
    }

    SECTION("Set hierarchically with dot operator") {
        std::string s = "pokemon.tails = 5\n";
        s += "pokemon.x.y = 56.4";
        auto dict = PomlParser::parse(s);
        REQUIRE(dict.has("pokemon"));
        auto x = dict.dump();
        printf("Contents:\n%s\n", x.c_str());
        REQUIRE(dict["pokemon"].has("tails"));
        REQUIRE(5 == dict["pokemon"]["tails"].asInt());
        REQUIRE(dict["pokemon"].has("x"));
        REQUIRE(dict["pokemon"]["x"].has("y"));
        REQUIRE(56.4 == dict["pokemon"]["x"]["y"].asDouble());
    }

    SECTION("Arrays") {
        std::string s = "winners = []\n";
        auto dict = PomlParser::parse(s);
        REQUIRE(dict.has("winners"));
        REQUIRE(dict["winners"].asInts().size() == 0);
    }

    SECTION("comments on newlines") {
        std::string s = "title = \"TOML Example\" # comment\n";
        auto dict = PomlParser::parse(s);
        REQUIRE(1 == dict.count("title"));
        REQUIRE("TOML Example" == dict["title"].asString());
    }
}