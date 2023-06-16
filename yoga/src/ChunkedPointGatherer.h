#pragma once
#include <parfait/Point.h>
#include <parfait/LinearPartitioner.h>
#include <MessagePasser/MessagePasser.h>

namespace YOGA {

class ChunkedPointGatherer {
  public:
    ChunkedPointGatherer(MessagePasser mp,const std::vector<Parfait::Point<double>>& points,long max_chunk_size)
        :mp(mp),
         points(points),
         max_chunk_size(max_chunk_size),
         total_points(mp.ParallelSum(long(points.size())))
    {
    }

    std::vector<Parfait::Point<double>> extractChunkInParallel(int chunk){
        auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk,total_points,chunkCount());
        return mp.Gather(points, range.start, range.end);
    }

    int chunkCount(){
        return total_points / max_chunk_size + 1;
    }

  private:
    MessagePasser mp;
    const std::vector<Parfait::Point<double>>& points;
    const long max_chunk_size;
    const long total_points;

};

}