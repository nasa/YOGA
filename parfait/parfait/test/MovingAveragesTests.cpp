#include <RingAssertions.h>
#include <parfait/MovingAverages.h>

TEST_CASE("Moving Averages Exist") {
    int window_width = 10;
    Parfait::MovingAverages averages(window_width);
    SECTION("Empty history returns nan average") { REQUIRE(std::isnan(averages.simpleAverage())); }
    SECTION("One value average is that value") {
        averages.push(10);
        REQUIRE(averages.simpleAverage() == 10);
        REQUIRE(averages.exponentialAverage() == 10);
    }
    SECTION("Beyond the window doesn't matter") {
        averages.push(100);
        for (int i = 0; i < window_width; i++) {
            averages.push(10.0);
        }
        REQUIRE(averages.simpleAverage() == 10);
        REQUIRE(averages.exponentialAverage() == 10);
    }
}
