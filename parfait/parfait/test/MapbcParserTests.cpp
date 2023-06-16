#include <RingAssertions.h>
#include <parfait/MapbcReader.h>
#include <parfait/MapbcParser.h>

using namespace Parfait;

TEST_CASE("parse mapbc contents") {
    std::string mapbc = "3 some extra words/comments\n";
    mapbc += "1 300 name\n";
    mapbc += "2 400 other-name\n";
    mapbc += "3 500 name with spaces\n";

    auto bc_map = MapbcParser::parse(mapbc);
    REQUIRE(3 == bc_map.size());
    REQUIRE(bc_map.count(1) == 1);
    REQUIRE(300 == bc_map[1].first);
    REQUIRE("name" == bc_map[1].second);
    REQUIRE(bc_map.count(2) == 1);
    REQUIRE(400 == bc_map[2].first);
    REQUIRE("other-name" == bc_map[2].second);
    REQUIRE(bc_map.count(3) == 1);
    REQUIRE(500 == bc_map[3].first);
    REQUIRE("namewithspaces" == bc_map[3].second);
}
