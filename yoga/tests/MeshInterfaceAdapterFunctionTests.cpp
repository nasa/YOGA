#include <RingAssertions.h>
#include "RankTranslator.h"

TEST_CASE("Calc global rank from local rank and nranks per communicator") {
    std::vector<int> ranks_per_comm{1, 20, 35};

    RankTranslator<int> translator(ranks_per_comm);

    int local_rank = 0;
    int communicator = 0;
    int global_rank = translator.globalRank(local_rank, communicator);
    REQUIRE(0 == global_rank);

    REQUIRE(1 == translator.globalRank(0, 1));
    REQUIRE(3 == translator.globalRank(2, 1));
    REQUIRE(21 == translator.globalRank(0, 2));
    REQUIRE(55 == translator.globalRank(34, 2));
    REQUIRE_THROWS(translator.globalRank(35, 2));
    REQUIRE_THROWS(translator.globalRank(0, 3));
}
