#include <RingAssertions.h>
#include <parfait/Wiggle.h>
#include <parfait/Point.h>

using namespace Parfait;

TEST_CASE("wiggle") {
    Point<double> p{0, 0, 0};
    Wiggler wiggler;
    double distance = 1.0;
    auto before = p;
    wiggler.wiggle(p, distance);
    REQUIRE(before.distance(p) == distance);
}