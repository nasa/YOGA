#include <RingAssertions.h>
#include <array>
#include <parfait/LinearAlgebra.h>

TEST_CASE("Linear Algebra solve 2x2") {
    using namespace Parfait;

    std::array<double, 4> A = {1.0, 2.0, 3.0, 4.0};
    std::array<double, 2> b = {2.0, 3.0};
    std::array<double, 2> x = LinearAlgebra::solve(A, b);
    REQUIRE(x[0] == Approx(-1.0));
    REQUIRE(x[1] == Approx(3.0 / 2.0));
}
