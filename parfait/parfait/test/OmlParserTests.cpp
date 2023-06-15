#include <RingAssertions.h>
#include <parfait/OmlParser.h>

TEST_CASE("Can parse json from root oml parser") {
    std::string json = R"({"dog":"tayla"})";
    REQUIRE_NOTHROW(Parfait::OmlParser::parse(json));
}
TEST_CASE("Can parse poml from root oml parser") {
    std::string poml = R"(dog = "tayla")";
    REQUIRE_NOTHROW(Parfait::OmlParser::parse(poml));
}
