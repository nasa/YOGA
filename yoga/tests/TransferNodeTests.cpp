#include <parfait/Point.h>
#include <RingAssertions.h>
#include "TransferNode.h"

using namespace YOGA;

TEST_CASE("work node has the right stuff in it") {
    const long globalId = 1;
    const Parfait::Point<double> p{0, 1, 2};
    const int owningRank = 3;
    const int associatedComponentId = 4;
    TransferNode node(globalId, p, 0.0, associatedComponentId, owningRank);
    REQUIRE(globalId == node.globalId);
    REQUIRE(p.approxEqual(node.xyz));
    REQUIRE(owningRank == node.owningRank);
    REQUIRE(associatedComponentId == node.associatedComponentId);
}
