#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <chrono>
#include <thread>

TEST_CASE("See what's out there"){
    MessagePasser mp(MPI_COMM_WORLD);

    bool is_message_out_there = false;

    auto probe = mp.Probe();
    is_message_out_there = probe.hasMessage();

    REQUIRE_FALSE(is_message_out_there);

    int n = 5;
    auto status = mp.NonBlockingSend(n, mp.Rank());

    for (int i = 0; i < 5; i++) {
        probe = mp.Probe();
        is_message_out_there = probe.hasMessage();
        if (is_message_out_there)
            break;
        printf("Probing for message....\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    REQUIRE(is_message_out_there);
    int source_rank = probe.sourceRank();
    REQUIRE(mp.Rank() == source_rank);

    int m;
    mp.Recv(m, source_rank);

    status.wait();
}
