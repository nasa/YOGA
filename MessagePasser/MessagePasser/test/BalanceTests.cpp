#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("Balance to all ranks") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<int> items;
    if(mp.Rank() == 0){
        items.resize(100);
    }
    size_t total = mp.ParallelSum(items.size());

    size_t count = total / static_cast<size_t>(mp.NumberOfProcesses());
    size_t remainder = total % static_cast<size_t>(mp.NumberOfProcesses());
    for (int r = 0; static_cast<size_t>(r) < remainder; ++r) {
      if (mp.Rank() == r) count++;
    }

    auto balanced = mp.Balance(items);
    REQUIRE(mp.ParallelSum(balanced.size()) == total);
    REQUIRE(balanced.size() == count);
}

TEST_CASE("shift to range"){
    std::vector<size_t> a = {1,2,3,4,5,6,0,0,0,0,0,0};
    MessagePasserImpl::shiftToRequestedRankRange(a, 2);
    REQUIRE(a[2] == 1);
    REQUIRE(a[3] == 2);
    REQUIRE(a[4] == 3);
    REQUIRE(a[5] == 4);
    REQUIRE(a[6] == 5);
    REQUIRE(a[7] == 6);
    REQUIRE(a[8] == 0);
    REQUIRE(a[9] == 0);
    REQUIRE(a[10] == 0);
    REQUIRE(a[11] == 0);
    REQUIRE(a[0] == 0);
    REQUIRE(a[1] == 0);
}

TEST_CASE("shift sparse vector to range"){
    std::vector<size_t> proc_counts {1,0};
    int rank_range_start = 1;
    int rank_range_end = 2;
    int num_target_ranks = rank_range_end - rank_range_start;

    auto total_count = std::accumulate(proc_counts.begin(), proc_counts.end(), size_t(0));
    auto equal_counts = MessagePasserImpl::roughlyEqualCounts(total_count, static_cast<size_t>(num_target_ranks));

    // previously, Balance called this shift call:
    //MessagePasserImpl::shiftToRequestedRankRange(equal_counts, rank_range_start);


    // but I think it should just get the offset from the start of the range
    long gid = 0;
    auto owner = MessagePasserImpl::findOwner(equal_counts, static_cast<long>(gid));
    // then we shift it post-priori to get the actual rank
    owner += static_cast<size_t>(rank_range_start);

    // This unit test is a duplication of what I implemented in Balance() to
    // give us a simple example that demonstrates how I *think* it should work
    // *if* I'm understanding the rest of the process.

    REQUIRE(owner == 1);
}

TEST_CASE("Balance to 1 rank (yes, this is basically just a gather)") {
    MessagePasser mp(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() < 2) return;
    std::vector<int> items;
    if(mp.Rank() == 0){
        items.resize(100);
    }
    auto balanced = mp.Balance(items, 1, 2);
    if(mp.Rank() == 1)
        REQUIRE(balanced.size() == 100);
    else
        REQUIRE(balanced.size() == 0);
}
