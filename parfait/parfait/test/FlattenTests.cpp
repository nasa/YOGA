#include <RingAssertions.h>
#include <set>
#include <parfait/Flatten.h>

TEST_CASE("Flatten vec of vec") {
    std::vector<std::vector<int>> things = {{1}, {2}, {3}};
    auto flat = Parfait::flatten(things);
    REQUIRE(flat.size() == 3);
    REQUIRE(flat[0] == 1);
    REQUIRE(flat[1] == 2);
    REQUIRE(flat[2] == 3);
}
