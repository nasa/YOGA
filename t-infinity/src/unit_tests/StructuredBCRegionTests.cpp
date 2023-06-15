#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <t-infinity/StructuredBCRegion.h>
#include <t-infinity/Extract.h>
#include "t-infinity/CartMesh.h"
#include "t-infinity/SMesh.h"
#include "t-infinity/StructuredToUnstructuredMeshAdapter.h"
#include "t-infinity/FaceNeighbors.h"

TEST_CASE("Can parse structured bc_regions from Vulcan string") {
    std::string patch_string = "INFLOW  1  I  MIN  J  MIN MAX  K  MIN  MAX";

    inf::StructuredBCRegion bc_region = inf::StructuredBCRegion::parse(patch_string);
    REQUIRE(bc_region.name == "INFLOW");
    REQUIRE(bc_region.host_block == 0);
    REQUIRE(bc_region.side == inf::BlockSide::IMIN);
    REQUIRE(bc_region.axis1 == inf::BlockAxis::J);
    REQUIRE(bc_region.axis1_range[0] == inf::StructuredBCRegion::MIN);
    REQUIRE(bc_region.axis1_range[1] == inf::StructuredBCRegion::MAX);

    REQUIRE(bc_region.axis2 == inf::BlockAxis::K);
    REQUIRE(bc_region.axis2_range[0] == -2);
    REQUIRE(bc_region.axis2_range[1] == -1);
}

TEST_CASE("Can parse a subset bc_region") {
    std::string bc_region_string = "my-name-is-slim-shady  8  J  MAX  I  4 6  K  MIN  MAX";

    inf::StructuredBCRegion bc_region = inf::StructuredBCRegion::parse(bc_region_string);
    REQUIRE(bc_region.name == "my-name-is-slim-shady");
    REQUIRE(bc_region.host_block == 7);
    REQUIRE(bc_region.side == inf::BlockSide::JMAX);
    REQUIRE(bc_region.axis1 == inf::BlockAxis::I);
    REQUIRE(bc_region.axis1_range[0] == 3);
    REQUIRE(bc_region.axis1_range[1] == 5);

    REQUIRE(bc_region.axis2 == inf::BlockAxis::K);
    REQUIRE(bc_region.axis2_range[0] == inf::StructuredBCRegion::MIN);
    REQUIRE(bc_region.axis2_range[1] == inf::StructuredBCRegion::MAX);
}

TEST_CASE("Throw if line doesn't match length requirements") {
    std::string bad_line = "too short";
    REQUIRE_THROWS(inf::StructuredBCRegion::parse(bad_line));
}

TEST_CASE("Parse a vector of bc regions, skipping comments") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    std::string patch_string = R"(
imin  1  I  MIN  J  MIN MAX  K  MIN  MAX
imax  1  I  MAX  J  MIN MAX  K  MIN  MAX
jmin  1  J  MIN  I  MIN MAX  K  MIN  MAX
 # haha!  A wild comment appears
jmaxp1  1  J  MAX  I  1 2  K  MIN  MAX
jmaxp2  1  J  MAX  I  2 3  K  MIN  MAX
kmin  1  K  MIN  I  MIN MAX  J  MIN  MAX
kmax  1  K  MAX  I  MIN MAX  J  MIN  MAX
)";

    auto regions = inf::StructuredBCRegion::parse(Parfait::StringTools::split(patch_string, "\n"));
    REQUIRE(regions.size() == 7);

    auto smesh = std::make_shared<inf::SMesh>();
    auto block = std::make_shared<inf::CartesianBlock>(std::array<int, 3>{2, 2, 2});
    smesh->add(block);

    auto umesh = inf::convertStructuredMeshToUnstructuredMesh(mp, *smesh);
    inf::StructuredBCRegion::reassignBoundaryTagsFromBCRegions(*umesh, regions);

    auto tags = inf::extract2DTags(*umesh);
    REQUIRE(tags.size() == 7);
}

TEST_CASE("Get a triple loop range for a bc region"){
    SECTION("IMIN face") {
        std::string bc_region_string = "INFLOW  1  I  MIN  J  1 5  K  2  4";
        auto region = inf::StructuredBCRegion::parse(bc_region_string);
        REQUIRE(region.axis1_range[0] == 0);
        REQUIRE(region.axis1_range[1] == 4);
        std::array<int, 3> node_dimensions = {10, 5, 10};
        auto start = region.start(node_dimensions);
        REQUIRE(start[0] == 0);
        REQUIRE(start[1] == 0);
        REQUIRE(start[2] == 1);
        auto end = region.end(node_dimensions);
        REQUIRE(end[0] == 1);
        REQUIRE(end[1] == 4);
        REQUIRE(end[2] == 3);
    }
    SECTION("IMAX face"){
        std::string bc_region_string = "INFLOW  1  I  MAX  J  1 10  K  2  4";
        auto region = inf::StructuredBCRegion::parse(bc_region_string);
        std::array<int, 3> node_dimensions = {10, 10, 10};
        auto start = region.start(node_dimensions);
        REQUIRE(start[0] == 8);
        REQUIRE(start[1] == 0);
        REQUIRE(start[2] == 1);
        auto end = region.end(node_dimensions);
        REQUIRE(end[0] == 9);
        REQUIRE(end[1] == 9);
        REQUIRE(end[2] == 3);
    }

    SECTION("IMAX face with min and max in the ranges"){
        std::string bc_region_string = "INFLOW  1  I  MAX  J  1 10  K  MIN  MAX";
        auto region = inf::StructuredBCRegion::parse(bc_region_string);
        std::array<int, 3> node_dimensions = {10, 10, 10};
        auto start = region.start(node_dimensions);
        REQUIRE(start[0] == 8);
        REQUIRE(start[1] == 0);
        REQUIRE(start[2] == 0);
        auto end = region.end(node_dimensions);
        REQUIRE(end[0] == 9);
        REQUIRE(end[1] == 9);
        REQUIRE(end[2] == 9);
    }

    SECTION("KMAX face"){
        std::string bc_region_string = "INFLOW  1  K  MAX  I  1 10  J  2  4";
        auto region = inf::StructuredBCRegion::parse(bc_region_string);
        std::array<int, 3> node_dimensions = {12, 12, 23};
        auto start = region.start(node_dimensions);
        REQUIRE(start[0] == 0);
        REQUIRE(start[1] == 1);
        REQUIRE(start[2] == 21);
        auto end = region.end(node_dimensions);
        REQUIRE(end[0] == 9);
        REQUIRE(end[1] == 3);
        REQUIRE(end[2] == 22);
    }
}