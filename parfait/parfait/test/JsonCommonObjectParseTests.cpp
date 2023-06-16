#include <RingAssertions.h>
#include <parfait/JsonCommon.h>

TEST_CASE("Can parse an extent box") {
    using namespace Parfait;
    std::string config = R"({
"lo":[1,1,1],
"hi":[2,2,3]
})";

    auto json = Parfait::JsonParser::parse(config);

    Parfait::Extent<double> e = Parfait::Json::Common::extent(json);
    REQUIRE(e.lo[0] == Approx(1.0));
    REQUIRE(e.lo[1] == Approx(1.0));
    REQUIRE(e.lo[2] == Approx(1.0));
    REQUIRE(e.hi[0] == Approx(2.0));
    REQUIRE(e.hi[1] == Approx(2.0));
    REQUIRE(e.hi[2] == Approx(3.0));
}

TEST_CASE("Can parse a sphere") {
    using namespace Parfait;
    std::string config = R"({
"center":[1,2,2], "radius":2.0
})";

    auto json = Parfait::JsonParser::parse(config);

    Parfait::Sphere s = Parfait::Json::Common::sphere(json);
    REQUIRE(s.center[0] == Approx(1.0));
    REQUIRE(s.center[1] == Approx(2.0));
    REQUIRE(s.center[2] == Approx(2.0));
    REQUIRE(s.radius == Approx(2.0));
}
