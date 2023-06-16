#include <RingAssertions.h>
#include "DensityEstimator.h"

using namespace YOGA;

TEST_CASE("build tree from cartblock & counts"){
    Parfait::CartBlock block({0,0,0},{1,1,1},10,10,10 );
    std::vector<double> n_per_cell(block.numberOfCells(),5);

    DensityEstimator estimator(block,block.numberOfCells()/1000+1,n_per_cell);

    Parfait::Extent<double> e {{0,0,0},{0.5,1,1}};
    int n = estimator.estimateContainedNodeCount(e);

    int target = 2.5*block.numberOfCells();
    int error = std::abs(target-n);
    double ratio = error / double(target);

    REQUIRE(0.01 > ratio);

    int nbins = 1000;
    Parfait::CartBlock block2({0,0,0},{1,1,1},nbins,1,1);
    std::vector<int> bins(nbins);
    estimator.estimateContainedNodeCounts(block2,bins);

    int total = std::accumulate(bins.begin(),bins.end(),0);
    REQUIRE(5000 == total);
}

TEST_CASE("build tree from cartblock & counts with empty entry"){
    Parfait::CartBlock block({0,0,0},{1,1,1},10,10,10 );
    std::vector<double> n_per_cell(block.numberOfCells(),0);

    for(int i=0;i<block.numberOfCells();i++){
        if(i%10 == 0) n_per_cell[i] = 5000;
    }

    printf("create estimator\n"); fflush(stdout);
    DensityEstimator estimator(block,block.numberOfCells()/1000+1,n_per_cell);
    printf("done\n"); fflush(stdout);

    Parfait::Extent<double> e {{0,0,0},{0.5,1,1}};
    //int n = estimator.estimateContainedNodeCount(e);
    //REQUIRE(1250 == n);
    int n = estimator.estimateContainedNodeCount(block);
    REQUIRE(500000 == n);

    int nbins = 1000;
    Parfait::CartBlock block2({0,0,0},{1,1,1},nbins,1,1);
    std::vector<int> bins(nbins);
    estimator.estimateContainedNodeCounts(block2,bins);

    int total = std::accumulate(bins.begin(),bins.end(),0);
    REQUIRE(500000 == total);
}
