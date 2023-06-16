#include <RingAssertions.h>
#include "AlternateMapBuilder.h"

using namespace YOGA;

TEST_CASE("build map of alternate ranks for heavy partitions"){
    std::vector<long> vec = {140,3,0,0,
                             5,120,50,50,
                             21,52,50,130,
                             135,133,900,1};

    auto contains = [](auto& vec,auto val){
        return std::count(vec.begin(),vec.end(),val) == 1;
    };

    auto top = AlternateMapBuilder::getIndicesOfHighest(vec,2);
    REQUIRE(2 == top.size());
    REQUIRE(contains(top,0));
    REQUIRE(contains(top,14));

    auto bottom = AlternateMapBuilder::getIndicesOfLowest(vec,2);
    REQUIRE(2 == bottom.size());
    REQUIRE(contains(bottom,2));
    REQUIRE(contains(bottom,3));
}

TEST_CASE("test with zero"){
    std::vector<double> vec = { 44364.0, 63710.0, 68636.0, 53987.0};
    auto top = AlternateMapBuilder::getIndicesOfHighest(vec,0);
    REQUIRE(0 == top.size());
    auto bottom = AlternateMapBuilder::getIndicesOfLowest(vec,0);
    REQUIRE(0 == bottom.size());
}

