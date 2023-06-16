#include <RingAssertions.h>
#include <parfait/RegionFactory.h>

using namespace Parfait;
TEST_CASE("regionfactory throws if config is empty") { REQUIRE_THROWS(RegionFactory::create("")); }
TEST_CASE("regionfactory can create sphere happy-path one liner") {
    auto region = RegionFactory::create("sphere radius 2.2 center 1 2 3 end");
    auto sphere = dynamic_cast<SphereRegion*>(region.get());
    REQUIRE(sphere != nullptr);
    REQUIRE(sphere->radius == 2.2);
    REQUIRE(sphere->center[0] == 1.0);
    REQUIRE(sphere->center[1] == 2.0);
    REQUIRE(sphere->center[2] == 3.0);
}

TEST_CASE("regionfactory can create sphere happy-path with newlines") {
    auto region = RegionFactory::create("sphere\n radius 2.2\n center 1 2 3\nend");
    auto sphere = dynamic_cast<SphereRegion*>(region.get());
    REQUIRE(sphere != nullptr);
    REQUIRE(sphere->radius == 2.2);
    REQUIRE(sphere->center[0] == 1.0);
    REQUIRE(sphere->center[1] == 2.0);
    REQUIRE(sphere->center[2] == 3.0);
}

TEST_CASE("regionfactory throws if you forgot to specify radius") {
    REQUIRE_THROWS(RegionFactory::create("sphere\n center 1 2 3\nend"));
    REQUIRE_THROWS(RegionFactory::create("sphere\n radius 1.0 \nend"));
    REQUIRE_THROWS(RegionFactory::create("sphere\n radius center 0 0 0 \nend"));
    REQUIRE_THROWS(RegionFactory::create("sphere\n radius 2.0 center  0 \nend"));
}

TEST_CASE("region factory can create a cartesian box") {
    auto region = RegionFactory::create("cartesian box 0.1 0.1 0 1 1 1");
    auto extent = dynamic_cast<ExtentRegion*>(region.get());
    REQUIRE(extent != nullptr);
    REQUIRE(extent->extent.lo[0] == 0.1);
    REQUIRE(extent->extent.lo[1] == 0.1);
    REQUIRE(extent->extent.lo[2] == 0.0);
    REQUIRE(extent->extent.hi[0] == 1.0);
    REQUIRE(extent->extent.hi[1] == 1.0);
    REQUIRE(extent->extent.hi[2] == 1.0);
}

TEST_CASE("regionfactory can create a full hex") {
    auto region = RegionFactory::create("hex 0 0 0  1 0 0  1 1 0  0 1 0  0 0 1  1 0 1  1 1 1  0 1 1 ");
    auto hex = dynamic_cast<HexRegion*>(region.get());
    REQUIRE(hex != nullptr);
}

TEST_CASE("Split into multiple filters") {
    std::string config = "sphere radius 1.0 center 0 0 0 end sphere radius 2.0 center 1 1 1 end";
    auto object_configs = Parfait::StringTools::split(config, "end");
    REQUIRE(object_configs.size() == 2);
}

TEST_CASE("can create a composite region") {
    std::string config = "sphere radius 1.0 center 0 0 0 end sphere radius 2.0 center 1 1 1 end";
    auto region = RegionFactory::create(config);
    auto composite = dynamic_cast<CompositeRegion*>(region.get());
    REQUIRE(composite != nullptr);
}
