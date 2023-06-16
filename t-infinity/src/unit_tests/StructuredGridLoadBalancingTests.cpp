#include <RingAssertions.h>
#include <t-infinity/StructuredMeshLoadBalancing.h>

TEST_CASE("Measure perfect balance") {
    std::vector<double> work_per_rank = {1, 1, 1, 1, 1};
    auto balance = inf::measureLoadBalance(work_per_rank);
    REQUIRE(balance == Approx(1.));
}

TEST_CASE("Measure half balance") {
    std::vector<double> work_per_rank(1000, 1);
    work_per_rank[0] = 2;
    auto balance = inf::measureLoadBalance(work_per_rank);
    REQUIRE(balance == Approx(0.5).epsilon(0.01));
}

TEST_CASE("Round Robin assign work to ranks") {
    typedef int Item;
    std::map<Item, double> work_per_item;
    for (int i = 0; i < 100; i++) {
        work_per_item[i] = i;
    }

    int num_ranks = 10;
    auto items_to_ranks = inf::mapWorkToRanks(work_per_item, num_ranks);
    std::vector<double> work_per_rank(num_ranks, 0.0);
    for (const auto& p : items_to_ranks) {
        int item = p.first;
        int rank = p.second;
        work_per_rank[rank] += work_per_item.at(item);
    }
    REQUIRE(items_to_ranks.size() == work_per_item.size());

    auto load_balance = inf::measureLoadBalance(work_per_rank);
    REQUIRE(1.0 == Approx(load_balance));
}
