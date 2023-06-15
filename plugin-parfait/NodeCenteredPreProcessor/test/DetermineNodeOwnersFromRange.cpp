#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <Redistributor.h>

TEST_CASE("Figure out node owners based on range owned by each rank") {
    MessagePasser mp(MPI_COMM_WORLD);
    int rank = mp.Rank();
    long max_owned_id = (rank + 1) * 10 - 1;

    int nproc = mp.NumberOfProcesses();
    std::vector<long> some_global_nodes{0, 3, (nproc - 1) * 10, (nproc - 1) * 10 + 5};

    std::vector<int> owners = Redistributor::determineNodeOwners(mp, max_owned_id, some_global_nodes);

    REQUIRE(some_global_nodes.size() == owners.size());
    REQUIRE(0 == owners[0]);
    REQUIRE(0 == owners[1]);
    REQUIRE((nproc - 1) == owners[2]);
    REQUIRE((nproc - 1) == owners[3]);
}