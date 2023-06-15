#include <MessagePasser/MessagePasser.h>
#include <RingAssertions.h>

SCENARIO("To demonstrate parallel sum bug") {
    MessagePasser mp(MPI_COMM_WORLD);
    int const parallel_sum = mp.ParallelSum(mp.Rank(), 0);
    if (mp.Rank() == 0) {
        int sum = 0;
        for (int i = 0; i < mp.NumberOfProcesses(); i++) sum += i;
        REQUIRE(parallel_sum == sum);
    }
}
