#include <RingAssertions.h>
#include <array>
#include <vector>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("Automagically generate custom MPI_Datatypes at compile time"){
    MessagePasser mp(MPI_COMM_WORLD);

    MPI_Datatype mpi_type = mp.Type(int());
    MessagePasser::freeIfCustomType(mpi_type);
    REQUIRE(MPI_INT == mpi_type);
    mpi_type = mp.Type(float());
    MessagePasser::freeIfCustomType(mpi_type);
    REQUIRE(MPI_FLOAT == mpi_type);
    mpi_type = mp.Type(double());
    MessagePasser::freeIfCustomType(mpi_type);
    REQUIRE(MPI_DOUBLE == mpi_type);

    struct PokeMasterBelt{
        int number_of_pokeballs;
        int size;
        std::array<int,2> pouch_sizes;
    };

    // make sure you can query type more than once
    for(int i=0;i<5;i++) {
        REQUIRE_NOTHROW(mpi_type = mp.Type(PokeMasterBelt()));
        MessagePasser::freeIfCustomType(mpi_type);
    }

    SECTION("Make sure it can actually be communicated") {
        std::vector<PokeMasterBelt> belts;
        if (mp.Rank() == 0)
            belts.push_back({5, 32, {4, 4}});
        mp.Broadcast(belts, 0);
        REQUIRE(belts.size() == 1);
        REQUIRE(belts.front().size == 32);
        REQUIRE(belts.front().number_of_pokeballs == 5);
        REQUIRE(belts.front().pouch_sizes[0] == 4);
        REQUIRE(belts.front().pouch_sizes[1] == 4);
    }

    SECTION("Make sure an empty one can be sent"){
        std::vector<PokeMasterBelt> belts;
        mp.Broadcast(belts,0);
        REQUIRE(belts.size() == 0);
    }
}
