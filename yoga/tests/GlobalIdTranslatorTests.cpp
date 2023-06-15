#include <RingAssertions.h>
#include "GlobalIdTranslator.h"

TEST_CASE("number of ranks per communicator/component mesh") {
    std::vector<int> component_id_for_rank{0, 0, 0, 0, 1, 1, 1};
    std::vector<int> ranks_per_component = GlobalIdTranslator::calcRanksPerComponent(component_id_for_rank);
    REQUIRE(2 == ranks_per_component.size());
    REQUIRE(4 == ranks_per_component[0]);
    REQUIRE(3 == ranks_per_component[1]);
}

TEST_CASE("Given nodes per rank, calc global id") {
    std::vector<long> nodes_per_domain{55, 75, 260};
    GlobalIdTranslator translator(nodes_per_domain);

    REQUIRE(3 == translator.globalId(3, 0));
    REQUIRE(4 == translator.globalId(4, 0));
    REQUIRE(55 == translator.globalId(0, 1));
    REQUIRE(137 == translator.globalId(7, 2));
}
