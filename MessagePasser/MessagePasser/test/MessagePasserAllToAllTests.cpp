#include <vector>
#include <array>
#include <map>
#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>

int calcRightNeighbor(int rank, int nranks) { return (rank + 1) % nranks; }

int calcLeftNeighbor(int rank, int nranks) {
    int left_nbr_rank = rank - 1;
    if (left_nbr_rank == -1) left_nbr_rank = nranks - 1;
    return left_nbr_rank;
}

TEST_CASE("Can exchange a map of vector of packables") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    class Pokemon {
      public:
        std::string name;
        std::vector<int> move_ids;
    };

    std::map<int, std::vector<Pokemon>> pokemon;
    pokemon[0].push_back({"pikachu", {7, 11, 42}});
    pokemon[0].push_back({"Mew", {151}});

    auto packer = [](MessagePasser::Message& m, const Pokemon& p) {
        m.pack(p.name);
        m.pack(p.move_ids);
    };

    auto unpacker = [](MessagePasser::Message& m, Pokemon& p) {
        m.unpack(p.name);
        m.unpack(p.move_ids);
    };

    auto recv_pokemon = mp.Exchange(pokemon, packer, unpacker);
    if (mp.Rank() == 0) {
        REQUIRE(recv_pokemon[0][0].name == "pikachu");
        REQUIRE(recv_pokemon[0][0].move_ids == std::vector<int>{7, 11, 42});
        REQUIRE(recv_pokemon[0][1].name == "Mew");
        REQUIRE(recv_pokemon[0][1].move_ids == std::vector<int>{151});
    }
}

TEST_CASE("AllToAll with vectors") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<std::vector<int>> stuff_for_other_ranks(mp.NumberOfProcesses());
    int right_nbr_rank = calcRightNeighbor(mp.Rank(), mp.NumberOfProcesses());
    stuff_for_other_ranks[right_nbr_rank].push_back(mp.Rank() + 7);

    auto stuff_from_other_ranks = mp.Exchange(stuff_for_other_ranks);
    REQUIRE(mp.NumberOfProcesses() == int(stuff_for_other_ranks.size()));

    int left_nbr_rank = calcLeftNeighbor(mp.Rank(), mp.NumberOfProcesses());

    REQUIRE(stuff_from_other_ranks[left_nbr_rank].size() == 1);
    REQUIRE(stuff_from_other_ranks[left_nbr_rank].front() == left_nbr_rank + 7);
}

TEST_CASE("AllToAll with maps") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<int, std::vector<int>> stuff_for_other_ranks;
    int right_nbr_rank = calcRightNeighbor(mp.Rank(), mp.NumberOfProcesses());
    stuff_for_other_ranks[right_nbr_rank].push_back(mp.Rank() + 7);

    auto stuff_from_other_ranks = mp.Exchange(stuff_for_other_ranks);
    REQUIRE(1 == stuff_for_other_ranks.size());

    int left_nbr_rank = calcLeftNeighbor(mp.Rank(), mp.NumberOfProcesses());

    REQUIRE(stuff_from_other_ranks[left_nbr_rank].size() == 1);
    REQUIRE(stuff_from_other_ranks[left_nbr_rank].front() == left_nbr_rank + 7);
}

TEST_CASE("Prove that rvalues can be used (enforces const&)") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto stuff_from_other_ranks = mp.Exchange(std::map<int, std::vector<int>>({{0, {1, 2, 3, 4}}}));
    if (0 == mp.Rank()) {
        REQUIRE(mp.NumberOfProcesses() == int(stuff_from_other_ranks.size()));
        for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
            REQUIRE(1 == stuff_from_other_ranks[rank][0]);
            REQUIRE(2 == stuff_from_other_ranks[rank][1]);
            REQUIRE(3 == stuff_from_other_ranks[rank][2]);
            REQUIRE(4 == stuff_from_other_ranks[rank][3]);
        }
    }
}

TEST_CASE("Exchange arbitrary contiguous data") {
    MessagePasser mp(MPI_COMM_WORLD);
    struct Pokemon {
        std::array<double, 3> orientation;
        int number_of_tails;
        float attack_power;
        bool can_surf;
    };

    Pokemon pikachu{{1, 0, 0}, 1, 55.7f, true};
    Pokemon nine_tails{{0, 1, 0}, 9, 45.8f, false};
    Pokemon lapris{{0, 0, 1}, 1, 34.2f, true};

    std::map<int, std::vector<Pokemon>> pokemon_for_ranks;
    pokemon_for_ranks[0] = {pikachu, nine_tails, lapris};

    auto incoming_pokemon = mp.Exchange(std::move(pokemon_for_ranks));
    if (0 == mp.Rank()) {
        REQUIRE(mp.NumberOfProcesses() == int(incoming_pokemon.size()));
        for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
            REQUIRE(3 == incoming_pokemon[rank].size());
            REQUIRE(incoming_pokemon[rank][0].can_surf);
            REQUIRE_FALSE(incoming_pokemon[rank][1].can_surf);
            REQUIRE(incoming_pokemon[rank][2].can_surf);
        }
    }
}
