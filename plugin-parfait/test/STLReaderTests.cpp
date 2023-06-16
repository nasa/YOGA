#include <RingAssertions.h>
#include "../shared/STLReader.h"

std::string cube_filename = std::string(GRIDS_DIR) + "stl/unit-cube-scaled.stl";

TEST_CASE("STL Reader can give a node count") {
    STLReader reader(cube_filename);
    long node_count = reader.nodeCount();
    REQUIRE(node_count == 8);
}
