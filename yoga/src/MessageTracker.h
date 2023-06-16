#pragma once
#include <map>

namespace YOGA {
class MessageTracker {
  public:
    void incrementGridRequestCountForRank(int rank) {
        if (grid_requests_made.count(rank) == 0)
            grid_requests_made[rank] = 1;
        else
            grid_requests_made[rank]++;
    }
    void incrementDonorUpdateCountForRank(int rank) {
        if (donor_updates_made.count(rank) == 0)
            donor_updates_made[rank] = 1;
        else
            donor_updates_made[rank]++;
    }
    long getGridRequestCountForRank(int rank) {
        if (grid_requests_made.count(rank) == 1) return grid_requests_made.at(rank);
        return 0;
    }
    long getDonorUpdateCountForRank(int rank) {
        if (donor_updates_made.count(rank) == 1) return donor_updates_made.at(rank);
        return 0;
    }

  private:
    std::map<int, long> grid_requests_made;
    std::map<int, long> donor_updates_made;
};
}
