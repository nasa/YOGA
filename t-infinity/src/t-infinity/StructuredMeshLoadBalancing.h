#pragma once

#include <vector>
#include <map>
#include <queue>

namespace inf {
template <typename ItemId>
class Balancer {
  public:
    explicit Balancer(const std::map<ItemId, double>& work_per_item)
        : work_per_item(work_per_item) {}

    ItemId next() {
        auto n = biggest();
        work_per_item.erase(n);
        return n;
    }

    bool empty() { return work_per_item.empty(); }

  private:
    std::map<ItemId, double> work_per_item;

    ItemId biggest() {
        ItemId id = 0;
        double cost = 0;
        for (auto& pair : work_per_item) {
            auto i = pair.first;
            double c = pair.second;
            if (c > cost) {
                id = i;
                cost = c;
            }
        }
        return id;
    }
};

struct RankToWork {
    int rank;
    double amount_of_work;
    bool operator<(const RankToWork& b) const { return amount_of_work > b.amount_of_work; }
};

template <typename ItemId>
std::map<ItemId, int> mapWorkToRanks(const std::map<ItemId, double>& work_per_item, int num_ranks) {
    std::map<ItemId, int> items_to_ranks;
    Balancer<ItemId> balancer(work_per_item);

    std::priority_queue<RankToWork> q;

    for (int r = 0; r < num_ranks; ++r) q.push({r, 0.0});

    while (not balancer.empty()) {
        auto rank_to_work = q.top();
        auto item = balancer.next();
        items_to_ranks[item] = rank_to_work.rank;
        q.pop();
        rank_to_work.amount_of_work += work_per_item.at(item);
        q.push(rank_to_work);
    }

    return items_to_ranks;
}

inline double measureLoadBalance(const std::vector<double>& work_per_rank) {
    double max = 0;
    double sum = 0;
    for (auto w : work_per_rank) {
        max = std::max(w, max);
        sum += w;
    }
    double avg = sum / (double)work_per_rank.size();
    return avg / max;
}
}
