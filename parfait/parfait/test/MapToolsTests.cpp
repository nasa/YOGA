#include <RingAssertions.h>
#include <parfait/MapTools.h>

TEST_CASE("Can invert a local to global map") {
    std::vector<long> local_to_global = {7, 9990, 9};

    auto g2l = Parfait::MapTools::invert(local_to_global);
    REQUIRE(g2l.size() == 3);
    REQUIRE(g2l.at(7) == 0);
    REQUIRE(g2l.at(9990) == 1);
    REQUIRE(g2l.at(9) == 2);
}

TEST_CASE("Can invert a map") {
    std::map<std::string, int> animal_to_rank = {{"dog", 1}, {"cat", 99}, {"bird", -100}};

    auto rank_to_animal = Parfait::MapTools::invert(animal_to_rank);
    REQUIRE(rank_to_animal.size() == 3);
    REQUIRE(rank_to_animal.at(1) == "dog");
    REQUIRE(rank_to_animal.at(99) == "cat");
    REQUIRE(rank_to_animal.at(-100) == "bird");
}
