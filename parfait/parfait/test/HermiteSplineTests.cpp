#include <RingAssertions.h>
#include <parfait/HermiteSpline.h>

TEST_CASE("Create spline from points and derivatives") {
    using namespace Parfait;

    SECTION("linear function") {
        auto linear_function = [](double u) { return Parfait::Point<double>(u, 0, 0); };
        Parfait::Point<double> gradient{1, 0, 0};
        auto p0 = linear_function(0);
        auto p1 = linear_function(7);
        HermiteSpline h(p0, p1, gradient, gradient);
        REQUIRE(h.evaluate(0.0).approxEqual(linear_function(0)));
        REQUIRE(h.evaluate(0.5).approxEqual(linear_function(3.5)));
        REQUIRE(h.evaluate(1.0).approxEqual(linear_function(7)));
        auto expected_length = (p1 - p0).magnitude();
        REQUIRE(expected_length == Approx(h.integrate(0, 1)));

        for (int i = 0; i < 10; i++) {
            auto a = h.evaluate(h.calcUAtFractionOfLength(.1 * i));
            auto b = h.evaluate(h.calcUAtFractionOfLength(.1 * (i + 1)));
            double segment_length = (b - a).magnitude();
            double expected_segment_length = expected_length / 10.0;
            REQUIRE(expected_segment_length == Approx(segment_length));
        }
    }

    SECTION("cubic function") {
        auto cubic_function = [](double u) { return Parfait::Point<double>(5 * u * u * u + 3.6, 2 * u * u, 3 * u); };
        auto grad = [](double u) { return Parfait::Point<double>(3 * 5 * u * u, 2 * 2 * u, 3); };
        auto p0 = cubic_function(0);
        auto p1 = cubic_function(4);
        auto grad0 = grad(0);
        auto grad1 = grad(4);
        HermiteSpline h(p0, p1, grad0, grad1);
        REQUIRE(h.evaluate(0.0).approxEqual(p0));
        REQUIRE(h.evaluate(1.0).approxEqual(p1));
        SECTION("Make sure integration is internally consistent") {
            double total_length = h.integrate(0, 1);
            int n = 10;
            double du = 1.0 / double(n);
            double sum = 0.0;
            for (int i = 0; i < n; i++) {
                double segment_length = h.integrate(du * i, du * (i + 1));
                sum += segment_length;
            }
            REQUIRE(sum == Approx(total_length));
        }

        SECTION("calculate parametric coordinate that will give a fraction of the total length") {
            REQUIRE(0.0 == h.calcUAtFractionOfLength(0.0));
            double u_half = h.calcUAtFractionOfLength(.5);
            REQUIRE((0.5 * h.integrate(0, 1)) == Approx(h.integrate(0, u_half)));
            REQUIRE(1.0 == Approx(h.calcUAtFractionOfLength(1.0)));
        }
    }
}
