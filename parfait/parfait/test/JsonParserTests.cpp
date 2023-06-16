#include <RingAssertions.h>
#include <parfait/JsonParser.h>

using namespace Parfait;

bool matches_roundtrip(std::string s) {
    auto result = JsonParser::parse(s).dump();
    if (s != result) printf("Expected: %s\n   got:   %s\n", s.c_str(), result.c_str());
    return s == result;
}

TEST_CASE("Empty string returns empty Dictionary") { REQUIRE("{}" == JsonParser::parse("").dump()); }

TEST_CASE("Parse json string into json object") {
    SECTION("throw on unexpected token order") {
        REQUIRE_THROWS(JsonParser::parse("{"));
        REQUIRE_THROWS(JsonParser::parse("}"));
        REQUIRE_THROWS(JsonParser::parse("{[}"));
        REQUIRE_THROWS(JsonParser::parse("{]}"));
        REQUIRE_THROWS(JsonParser::parse("{:}"));
        REQUIRE_THROWS(JsonParser::parse("{\"dog\"}"));
        REQUIRE_THROWS(JsonParser::parse("{4}"));
        REQUIRE_THROWS(JsonParser::parse("{\"dog\":}"));
    }

    SECTION("verify simple key-value pairs") {
        REQUIRE(matches_roundtrip("{}"));
        REQUIRE(matches_roundtrip("{\"dog\":4}"));
        REQUIRE(matches_roundtrip("{\"cat\":5}"));
        REQUIRE(matches_roundtrip("{\"name\":\"ralph\"}"));
        REQUIRE(matches_roundtrip("{\"truth\":true}"));
        REQUIRE(matches_roundtrip("{\"a null\":null}"));
    }

    SECTION("verify nested objects") {
        REQUIRE(matches_roundtrip("{\"name\":\"pikachu\",\"size\":6}"));
        REQUIRE(matches_roundtrip("{\"pokemon\":{\"name\":\"pikachu\",\"size\":6}}"));
    }

    SECTION("verify arrays") {
        REQUIRE(matches_roundtrip("{\"empty array\":[]}"));
        REQUIRE(matches_roundtrip("{\"int array\":[0]}"));
        REQUIRE(matches_roundtrip("{\"int array\":[0,1,2]}"));

        REQUIRE(matches_roundtrip("{\"double array\":[3.14159]}"));

        REQUIRE(matches_roundtrip("{\"array of strings\":[\"ralph\",\"another string\"]}"));
        REQUIRE(matches_roundtrip("{\"array of arrays\":[[0],[1],[2,3,4]]}"));
        REQUIRE(matches_roundtrip("{\"array of booleans\":[true,false,false]}"));
    }

    SECTION("combination of nested and arrays") {
        std::string combo = "{\"boundary conditions\":[";
        combo += R"({"mesh boundary tag":2,"state":"farfield","type":"dirichlet"},)";
        combo += R"({"mesh boundary tag":1,"type":"tangency"})";
        combo += "]}";

        REQUIRE(matches_roundtrip(combo));
    }
}

TEST_CASE("Test that integers are stored with type integer") {
    auto dict = JsonParser::parse("{\"data\":1}");
    REQUIRE(dict.has("data"));
    REQUIRE(dict.at("data").type() == DictionaryEntry::Integer);
}
