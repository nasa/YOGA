#include <RingAssertions.h>
#include "AssemblyViaExchange.h"

using namespace YOGA;

TEST_CASE("split query point exchanges into chunks to reduce peak memory"){
    std::vector<long> query_point_counts {0,0,5,23040,500,246};
    long max_query_points_per_round = 400;
    int n_comm_rounds =  countRequiredCommRounds(max_query_points_per_round, query_point_counts);

    REQUIRE(58 == n_comm_rounds);

    // linear partitioner to divide my query points up each round, maybe just divide all evenly, even though
    // probably only a handful of ranks are overloaded
}