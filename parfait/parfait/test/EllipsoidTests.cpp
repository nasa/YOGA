#include <RingAssertions.h>
#include <parfait/Ellipsoid.h>
#include <parfait/DenseMatrix.h>
#include <parfait/CGNSElements.h>
#include <parfait/VTKWriter.h>

using namespace Parfait;

class EllipsoidBuilder {
  public:
    template <typename T>
    static Ellipsoid<T> fitEllipsoid(const std::vector<Point<T>>& points) {
        Point<T> c = calcCentroid(points);
        Point<T> r1 = calcLongestAxis(c, points);
        double r = r1.magnitude();
        printf("Centroid: %s   r: %lf\n", c.to_string().c_str(), r);
        return Ellipsoid<T>(c, r, r, r);
    }

  private:
    template <typename T>
    static Point<T> calcCentroid(const std::vector<Point<T>>& points) {
        Point<T> centroid{0, 0, 0};
        for (auto& p : points) centroid += p;
        return centroid / double(points.size());
    }

    template <typename T>
    static Point<T> calcLongestAxis(const Point<T>& centroid, const std::vector<Point<T>>& points) {
        Point<T> axis{0, 0, 0};
        for (auto& p : points) {
            Point<T> v = p - centroid;
            if (v.magnitude() > axis.magnitude()) axis = v;
        }
        return axis;
    }
};

TEST_CASE("Ellipsoid containment") {
    Point<double> origin{0, 0, 0};
    SECTION("ellipse at origin") {
        Ellipsoid<double> e(origin, 3, 2, 1);

        REQUIRE(e.contains({0, 0, 0}));
        REQUIRE(e.contains({3, 0, 0}));
        REQUIRE(e.contains({-3, 0, 0}));
        REQUIRE(e.contains({0, 2, 0}));
        REQUIRE(e.contains({0, -2, 0}));
        REQUIRE(e.contains({0, 0, 1}));
        REQUIRE(e.contains({0, 0, -1}));
        REQUIRE_FALSE(e.contains({3.1, 0, 0}));
    }

    SECTION("ellipse at non-origin") {
        Ellipsoid<double> e({20, 20, 20}, 3, 2, 1);

        REQUIRE(e.contains({20, 20, 20}));
        REQUIRE_FALSE(e.contains({0, 0, 0}));
    }

    SECTION("non-cartesian ellipse (rotated)") {
        Ellipsoid<double> e({0, 0, 0}, 3, 2, 1, 90, 0);

        REQUIRE(e.contains({0, 3, 0}));
        REQUIRE(e.contains({0, -3, 0}));
        REQUIRE(e.contains({2, 0, 0}));
        REQUIRE(e.contains({-2, 0, 0}));
        REQUIRE(e.contains({0, 0, 1}));
        REQUIRE(e.contains({0, 0, -1}));
        REQUIRE_FALSE(e.contains({3, 0, 0}));
    }
}
