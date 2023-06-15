#include <RingAssertions.h>
#include <parfait/SetTools.h>

TEST_CASE("Set union") {
    std::set<int> a{1, 2, 3};
    std::set<int> b{1, 5, 7};

    auto c = Parfait::SetTools::Union(a, b);
    REQUIRE(c.size() == 5);
    REQUIRE(c.count(1) == 1);
    REQUIRE(c.count(2) == 1);
    REQUIRE(c.count(3) == 1);
    REQUIRE(c.count(5) == 1);
    REQUIRE(c.count(7) == 1);
}

TEST_CASE("Set difference") {
    std::set<int> a{1, 2, 3};
    std::set<int> b{1, 5, 7};

    auto c = Parfait::SetTools::Difference(a, b);
    REQUIRE(c.size() == 2);
    REQUIRE(c.count(1) == 0);
    REQUIRE(c.count(2) == 1);
    REQUIRE(c.count(3) == 1);
}

TEST_CASE("Set intersection") {
    std::set<int> a{1, 2, 3};
    std::set<int> b{1, 3, 7};

    auto c = Parfait::SetTools::Intersection(a, b);
    REQUIRE(c.size() == 2);
    REQUIRE(c.count(1) == 1);
    REQUIRE(c.count(3) == 1);
}
