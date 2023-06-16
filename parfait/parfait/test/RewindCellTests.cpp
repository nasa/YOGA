#include <RingAssertions.h>
#include <parfait/Point.h>
#include <vector>
#include <parfait/RewindLeftHandedElements.h>

using namespace Parfait;

template <typename IndexContainer, typename PointContainer>
bool isTetInverted(const IndexContainer& tet, const PointContainer& points) {
    auto a = points[tet[0]];
    auto b = points[tet[1]];
    auto c = points[tet[2]];
    auto d = points[tet[3]];
    auto base_normal = Point<double>::cross((b - a), (c - a));
    base_normal.normalize();
    auto base_centroid = (1. / 3.) * (a + b + c);
    auto v = d - base_centroid;
    v.normalize();
    if (Point<double>::dot(v, base_normal) > 0.0)
        return false;
    else
        return true;
}

template <typename CellContainer, typename PointContainer>
bool isPyramidInverted(const CellContainer& pyramid, const PointContainer& points) {
    auto a = points[pyramid[0]];
    auto b = points[pyramid[1]];
    auto c = points[pyramid[2]];
    auto d = points[pyramid[3]];
    auto e = points[pyramid[4]];
    auto base_normal_right = Point<double>::cross((b - a), (c - a));
    base_normal_right.normalize();
    auto base_normal_left = Point<double>::cross((c - a), (d - a));
    base_normal_left.normalize();
    auto base_centroid = (1. / 4.) * (a + b + c + d);
    auto v = e - base_centroid;
    v.normalize();
    if (Point<double>::dot(v, base_normal_right) > 0.0 and Point<double>::dot(v, base_normal_left) > 0.0)
        return false;
    else
        return true;
}

template <typename CellContainer, typename PointContainer>
bool isPrismInverted(const CellContainer& prism, const PointContainer& points) {
    auto a = points[prism[0]];
    auto b = points[prism[1]];
    auto c = points[prism[2]];
    auto d = points[prism[3]];
    auto e = points[prism[3]];
    auto f = points[prism[3]];
    auto base_normal = Point<double>::cross((b - a), (c - a));
    base_normal.normalize();
    auto base_centroid = (1. / 3.) * (a + b + c);
    auto top_centroid = (1. / 3.) * (d + e + f);
    auto v = top_centroid - base_centroid;
    v.normalize();
    if (Point<double>::dot(v, base_normal) > 0.0)
        return false;
    else
        return true;
}

template <typename CellContainer, typename PointContainer>
bool isHexInverted(const CellContainer& hex, const PointContainer& points) {
    auto a = points[hex[0]];
    auto b = points[hex[1]];
    auto c = points[hex[2]];
    auto d = points[hex[3]];
    auto e = points[hex[4]];
    auto f = points[hex[5]];
    auto g = points[hex[6]];
    auto h = points[hex[7]];
    auto base_normal_right = Point<double>::cross((b - a), (c - a));
    base_normal_right.normalize();
    auto base_normal_left = Point<double>::cross((c - a), (d - a));
    base_normal_left.normalize();
    auto base_centroid = (1. / 4.) * (a + b + c + d);
    auto top_centroid = (1. / 4.) * (e + f + g + h);
    auto v = top_centroid - base_centroid;
    v.normalize();
    if (Point<double>::dot(v, base_normal_right) > 0.0 and Point<double>::dot(v, base_normal_left) > 0.0)
        return false;
    else
        return true;
}

static Parfait::Point<double> reflectPoint(const Parfait::Point<double>& p,
                                           const Parfait::Point<double>& plane_normal,
                                           double offset) {
    double a = plane_normal[0];
    double b = plane_normal[1];
    double c = plane_normal[2];
    double d = offset;
    double distance_to_plane = (a * p[0] + b * p[1] + c * p[2] + d) / std::sqrt(a * a + b * b + c * c);
    return p - 2.0 * distance_to_plane * plane_normal;
}

std::vector<Point<double>> reflectPoints(const std::vector<Point<double>>& points,
                                         const Point<double>& normal,
                                         double offset) {
    auto reflected_points = points;
    for (auto& p : reflected_points) p = reflectPoint(p, normal, offset);
    return reflected_points;
}

std::vector<Point<double>> reflectX(const std::vector<Point<double>>& points) {
    return reflectPoints(points, {1, 0, 0}, 0);
}

std::vector<Point<double>> reflectY(const std::vector<Point<double>>& points) {
    return reflectPoints(points, {0, 1, 0}, 0);
}

std::vector<Point<double>> reflectZ(const std::vector<Point<double>>& points) {
    return reflectPoints(points, {0, 0, 1}, 0);
}

TEST_CASE("rewind left-handed tetrahedron") {
    std::vector<Point<double>> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 0);
    points.emplace_back(0, 1, 0);
    points.emplace_back(0, 0, 1);

    std::vector<int> tetrahedron{0, 1, 2, 3};
    REQUIRE_FALSE(isTetInverted(tetrahedron, points));

    SECTION("reflect x") {
        auto reflected_points = reflectX(points);
        REQUIRE(isTetInverted(tetrahedron, reflected_points));
        rewindLeftHandedTet(tetrahedron);
        REQUIRE_FALSE(isTetInverted(tetrahedron, reflected_points));
    }

    SECTION("reflect y") {
        auto reflected_points = reflectY(points);
        REQUIRE(isTetInverted(tetrahedron, reflected_points));
        rewindLeftHandedTet(tetrahedron);
        REQUIRE_FALSE(isTetInverted(tetrahedron, reflected_points));
    }

    SECTION("reflect z") {
        auto reflected_points = reflectZ(points);
        REQUIRE(isTetInverted(tetrahedron, reflected_points));
        rewindLeftHandedTet(tetrahedron);
        REQUIRE_FALSE(isTetInverted(tetrahedron, reflected_points));
    }
}

TEST_CASE("rewind left-handed pyramid") {
    std::vector<Point<double>> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 0);
    points.emplace_back(1, 1, 0);
    points.emplace_back(0, 1, 0);
    points.emplace_back(0, 0, 1);

    std::vector<int> pyramid{0, 1, 2, 3, 4};
    REQUIRE_FALSE(isPyramidInverted(pyramid, points));

    SECTION("reflect x") {
        auto reflected_points = reflectX(points);
        REQUIRE(isPyramidInverted(pyramid, reflected_points));
        rewindLeftHandedPyramid(pyramid);
        REQUIRE_FALSE(isPyramidInverted(pyramid, reflected_points));
    }

    SECTION("reflect y") {
        auto reflected_points = reflectY(points);
        REQUIRE(isPyramidInverted(pyramid, reflected_points));
        rewindLeftHandedPyramid(pyramid);
        REQUIRE_FALSE(isPyramidInverted(pyramid, reflected_points));
    }

    SECTION("reflect z") {
        auto reflected_points = reflectZ(points);
        REQUIRE(isPyramidInverted(pyramid, reflected_points));
        rewindLeftHandedPyramid(pyramid);
        REQUIRE_FALSE(isPyramidInverted(pyramid, reflected_points));
    }
}

TEST_CASE("rewind left-handed prism") {
    std::vector<Point<double>> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 0);
    points.emplace_back(0, 1, 0);
    points.emplace_back(0, 0, 1);
    points.emplace_back(1, 0, 1);
    points.emplace_back(0, 1, 1);

    std::vector<int> prism{0, 1, 2, 3, 4, 5};

    REQUIRE_FALSE(isPrismInverted(prism, points));

    SECTION("reflect x") {
        auto reflected_points = reflectX(points);
        REQUIRE(isPrismInverted(prism, reflected_points));
        rewindLeftHandedPrism(prism);
        REQUIRE_FALSE(isPrismInverted(prism, reflected_points));
    }

    SECTION("reflect y") {
        auto reflected_points = reflectY(points);
        REQUIRE(isPrismInverted(prism, reflected_points));
        rewindLeftHandedPrism(prism);
        REQUIRE_FALSE(isPrismInverted(prism, reflected_points));
    }

    SECTION("reflect z") {
        auto reflected_points = reflectZ(points);
        REQUIRE(isPrismInverted(prism, reflected_points));
        rewindLeftHandedPrism(prism);
        REQUIRE_FALSE(isPrismInverted(prism, reflected_points));
    }
}

TEST_CASE("rewind left-handed hex") {
    std::vector<Point<double>> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 0);
    points.emplace_back(1, 1, 0);
    points.emplace_back(0, 1, 0);
    points.emplace_back(0, 0, 1);
    points.emplace_back(1, 0, 1);
    points.emplace_back(1, 1, 1);
    points.emplace_back(0, 1, 1);

    std::vector<int> hex{0, 1, 2, 3, 4, 5, 6, 7};

    REQUIRE_FALSE(isHexInverted(hex, points));

    SECTION("reflect x") {
        auto reflected_points = reflectX(points);
        REQUIRE(isHexInverted(hex, reflected_points));
        rewindLeftHandedHex(hex);
        REQUIRE_FALSE(isHexInverted(hex, reflected_points));
    }

    SECTION("reflect y") {
        auto reflected_points = reflectY(points);
        REQUIRE(isHexInverted(hex, reflected_points));
        rewindLeftHandedHex(hex);
        REQUIRE_FALSE(isHexInverted(hex, reflected_points));
    }

    SECTION("reflect Z") {
        auto reflected_points = reflectZ(points);
        REQUIRE(isHexInverted(hex, reflected_points));
        rewindLeftHandedHex(hex);
        REQUIRE_FALSE(isHexInverted(hex, reflected_points));
    }
}