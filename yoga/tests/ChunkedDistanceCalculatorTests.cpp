#include <RingAssertions.h>
#include "ChunkedPointGatherer.h"

using namespace YOGA;

TEST_CASE("gather part of surface"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> surface_points(2);
    long max_points_per_round = 1;

    ChunkedPointGatherer chunker(mp,surface_points,max_points_per_round);

    int expected_chunk_count = 2 * mp.NumberOfProcesses()+1;
    REQUIRE(expected_chunk_count == chunker.chunkCount());

    long total_points_processed = 0;
    for(int i=0;i<chunker.chunkCount();i++){
        auto chunk_points = chunker.extractChunkInParallel(i);
        total_points_processed += chunk_points.size();
    }

    long expected_total_points = 2 * mp.NumberOfProcesses();
    REQUIRE(expected_total_points == total_points_processed);
}