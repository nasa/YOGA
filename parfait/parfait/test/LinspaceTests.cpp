#include <RingAssertions.h>
#include <parfait/Linspace.h>

TEST_CASE("linspace with doubles") {
    auto dots = Parfait::linspace(0.0, 1.0, 10);
    REQUIRE(dots.size() == 10);
    REQUIRE(dots.front() == 0.0);
    REQUIRE(dots.back() == 1.0);
    double dx = 1.0 / static_cast<double>(10 - 1);
    for (int i = 0; i < 10; i++) {
        double x = i * dx + 0.0;
        REQUIRE(x == dots[i]);
    }
}

TEST_CASE("linspace with points") {
    Parfait::Point<double> start = {0, 0, 0};
    Parfait::Point<double> end = {1, 1, 1};
    auto dots = Parfait::linspace(start, end, 10);
    REQUIRE(dots.size() == 10);
    double dx = 1.0 / static_cast<double>(10 - 1);
    for (int i = 0; i < 10; i++) {
        Parfait::Point<double> x = i * Parfait::Point<double>{dx, dx, dx};
        REQUIRE(x == dots[i]);
    }
}
