
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#include <RingAssertions.h>
#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>

using namespace Parfait;

TEST_CASE("Extent, Constructor") {
    Point<double> lo(0, 0, 0), hi(1, 1, 1);
    Extent<double> v(lo, hi);

    REQUIRE(v.lo[0] == 0);
    REQUIRE(v.lo[1] == 0);
    REQUIRE(v.lo[2] == 0);

    REQUIRE(v.hi[0] == 1);
    REQUIRE(v.hi[1] == 1);
    REQUIRE(v.hi[2] == 1);
}

TEST_CASE(" dimension of extent box") {
    Parfait::Extent<double> e = {{0, 0, 0}, {2, 1, 1}};
    REQUIRE(e.longestDimension() == 0);
}

TEST_CASE("stl like functions") {
    Extent<int> extent{{0, 0, 0}, {1, 1, 1}};
    REQUIRE(extent.lo.data() == extent.data());
}

TEST_CASE("Distance from point to interior") {
    Extent<double> extent{{0, 0, 0}, {1, 1, 1}};
    double r2 = std::sqrt(2.0) / 2.0;
    // inside
    REQUIRE(0.0 == extent.distance({.5, .5, .5}));
    // closest to a corner
    REQUIRE(1.0 == Approx(extent.distance({1. + r2, 1 + r2, 0})));
    REQUIRE(1.0 == Approx(extent.distance({0, 1 + r2, 1 + r2})));
    REQUIRE(1.0 == Approx(extent.distance({1 + r2, 0, 1 + r2})));
    REQUIRE(1.0 == Approx(extent.distance({-r2, -r2, 0})));
    REQUIRE(1.0 == Approx(extent.distance({-r2, 0, -r2})));
    REQUIRE(1.0 == Approx(extent.distance({0, -r2, -r2})));
    // above, below, left, right, forward, back
    REQUIRE(1.0 == extent.distance({.5, .5, 2.0}));
    REQUIRE(1.0 == extent.distance({.5, .5, -1.0}));
    REQUIRE(1.0 == extent.distance({.5, 2, .5}));
    REQUIRE(1.0 == extent.distance({.5, -1, .5}));
    REQUIRE(1.0 == extent.distance({2, .5, .5}));
    REQUIRE(1.0 == extent.distance({-1, .5, .5}));

    // REQUIRE(1.0 == extent.distance({-1.0,.5,.5}));
}

TEST_CASE("Closest point in AABB") {
    Parfait::Extent<double> e{{10, 10, 10}, {11, 11, 11}};
    Parfait::Point<double> query{10.5, 10.5, 22};

    SECTION("snap z") {
        auto p = e.clamp({10.5, 10.5, 22});
        REQUIRE(p[0] == Approx(10.5));
        REQUIRE(p[1] == Approx(10.5));
        REQUIRE(p[2] == Approx(11));
    }
    SECTION("snap upper-corner") {
        auto p = e.clamp({22, 22, 22});
        REQUIRE(p[0] == Approx(11));
        REQUIRE(p[1] == Approx(11));
        REQUIRE(p[2] == Approx(11));
    }
    SECTION("snap lower-corner") {
        auto p = e.clamp({2, 2, 2});
        REQUIRE(p[0] == Approx(10));
        REQUIRE(p[1] == Approx(10));
        REQUIRE(p[2] == Approx(10));
    }
    SECTION("point inside voxel") {
        auto p = e.clamp({10.5, 10.5, 10.7});
        REQUIRE(p[0] == Approx(10.5));
        REQUIRE(p[1] == Approx(10.5));
        REQUIRE(p[2] == Approx(10.7));
    }
}

TEST_CASE("Extent, PointerConstructors") {
    double d_lo[3] = {0.1, 0.1, 0.1};
    double d_hi[3] = {1.1, 1.1, 1.1};

    Extent<double> d_extent(d_lo, d_hi);
    REQUIRE(0.1 == d_extent.lo[0]);
    REQUIRE(0.1 == d_extent.lo[1]);
    REQUIRE(0.1 == d_extent.lo[2]);

    REQUIRE(1.1 == d_extent.hi[0]);
    REQUIRE(1.1 == d_extent.hi[1]);
    REQUIRE(1.1 == d_extent.hi[2]);

    double extent_in[6] = {2.2, 3.3, 4.4, 5.5, 6.6, 7.7};
    Extent<double> e_extent(extent_in);
    REQUIRE(2.2 == e_extent.lo[0]);
    REQUIRE(3.3 == e_extent.lo[1]);
    REQUIRE(4.4 == e_extent.lo[2]);

    REQUIRE(5.5 == e_extent.hi[0]);
    REQUIRE(6.6 == e_extent.hi[1]);
    REQUIRE(7.7 == e_extent.hi[2]);
}

TEST_CASE("Extent, containsPoint") {
    Point<double> lo(0, 0, 0), hi(1, 1, 1);
    Extent<double> v(lo, hi);

    Point<double> p(0.25, 0.25, 0.25);
    REQUIRE(v.intersects(p));

    p = Point<double>(-1.0, 0.25, 0.0);
    REQUIRE(not v.intersects(p));
}


TEST_CASE("Extent, containsExtent") {
    Extent<double> box1({0, 0, 0}, {1, 1, 1});
    REQUIRE(box1.intersects(Extent<double>({0.1, 0.1, 0.1}, {0.2, 0.2, 0.2})));

    Extent<double> box2({-1, -1, -1}, {-0.5, -0.5, 0.5});
    REQUIRE(not box2.intersects(box1));
}

TEST_CASE("Extent, Length") {
    Extent<double> e({0, 0, 0}, {1, 1, 1});
    REQUIRE(0 == e.lo[0]);
    REQUIRE(0 == e.lo[1]);
    REQUIRE(0 == e.lo[2]);

    REQUIRE(1 == e.hi[0]);
    REQUIRE(1 == e.hi[1]);
    REQUIRE(1 == e.hi[2]);
}

TEST_CASE("Extent,resize") {
    Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    e.resize(2, 4, 8);

    REQUIRE(-.5 == e.lo[0]);
    REQUIRE(-1.5 == e.lo[1]);
    REQUIRE(-3.5 == e.lo[2]);
    REQUIRE(1.5 == e.hi[0]);
    REQUIRE(2.5 == e.hi[1]);
    REQUIRE(4.5 == e.hi[2]);
}

TEST_CASE("Extent,makeIsotropic") {
    Extent<double> e{{0, 0, 0}, {2, 1, 0}};
    e.makeIsotropic();
    REQUIRE(0 == e.lo[0]);
    REQUIRE(-.5 == e.lo[1]);
    REQUIRE(-1 == e.lo[2]);
    REQUIRE(2 == e.hi[0]);
    REQUIRE(1.5 == e.hi[1]);
    REQUIRE(1 == e.hi[2]);
}

TEST_CASE("Calc intersection of extents") {
    Extent<double> a{{0, 0, 0}, {5, 5, 5}};
    Extent<double> b{{1, 1, 1}, {3, 3, 3}};

    SECTION("One extents contains the other") {
        auto intersection = b.intersection(a);

        REQUIRE(1 == Approx(intersection.lo[0]));
        REQUIRE(1 == Approx(intersection.lo[1]));
        REQUIRE(1 == Approx(intersection.lo[2]));
        REQUIRE(3 == Approx(intersection.hi[0]));
        REQUIRE(3 == Approx(intersection.hi[1]));
        REQUIRE(3 == Approx(intersection.hi[2]));

        REQUIRE(intersection.approxEqual(a.intersection(b)));
    }

    SECTION("Extents intersect, but neither contains the other completely") {
        Extent<double> c{{2, 2, 2}, {4, 4, 4}};
        auto intersection = b.intersection(c);
        REQUIRE(2 == Approx(intersection.lo[0]));
        REQUIRE(2 == Approx(intersection.lo[1]));
        REQUIRE(2 == Approx(intersection.lo[2]));
        REQUIRE(3 == Approx(intersection.hi[0]));
        REQUIRE(3 == Approx(intersection.hi[1]));
        REQUIRE(3 == Approx(intersection.hi[2]));
    }
}

TEST_CASE("To string exists") {
    Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    REQUIRE_THAT(e.to_string(), Contains("lo"));
    REQUIRE_THAT(e.to_string(), Contains("hi"));
}

TEST_CASE("Line segment AABB intersections") {
    Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    REQUIRE(e.intersectsSegment({0, 0, 0}, {1, 1, 1}));                 // exactly within box
    REQUIRE(e.intersectsSegment({0.3, 0.3, 0.3}, {.7, .7, .7}));        // line segment completely within box
    REQUIRE(e.intersectsSegment({0.3, 0.3, 0.3}, {1.7, 1.7, 1.7}));     // line segment partially within box
    REQUIRE(e.intersectsSegment({-0.3, -0.3, -0.3}, {1.7, 1.7, 1.7}));  // box completely covered by line
    REQUIRE_FALSE(e.intersectsSegment({2, 2, 2}, {3, 2, 2}));           // not within box
    REQUIRE(e.intersectsSegment({1.5, 0.5, 0.5}, {-0.5, 0.75, 0.75}));  // clipping a corner

    REQUIRE_FALSE(e.intersectsSegment({-.5, 0.8, .5}, {.5, 1.5, .5}));
}

TEST_CASE("Extent, containsPoint and line segment intersection failure case") {
    std::vector<Point<double>> points{};
    points.push_back({-0.556045, -0.518724, -0.897422});
    points.push_back({-0.358076, -0.31437, 0.253686});
    points.push_back({-0.445565, 0.073387, -0.294082});
    points.push_back({-0.53667, 0.0311875, 0.345438});
    auto cell_extents = ExtentBuilder::build(points);
    Point<double> line_start{0.000109818, 8.24546e-08, 0.000998921};
    Point<double> line_stop{-1.0001, -0.000750901, 0.0108252};
    auto test_point = 0.5*(line_start + line_stop);
    REQUIRE(cell_extents.intersects(test_point));
    Extent<double> fake_extent{test_point,test_point};
    REQUIRE(cell_extents.intersects(fake_extent));
    REQUIRE(cell_extents.intersectsSegment(line_start, line_stop));
}