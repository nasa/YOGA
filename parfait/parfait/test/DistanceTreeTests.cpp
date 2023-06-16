
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
#include <parfait/Facet.h>
#include <limits>
#include <parfait/DistanceTree.h>
#include <parfait/Octree.h>
#include "parfait/PointGenerator.h"
#include "parfait/ExtentWriter.h"
#include "parfait/PointWriter.h"

TEST_CASE("DistanceTree::Node::isLeaf ") {
    Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree::Node node(e);
    REQUIRE(node.isLeaf());
}

TEST_CASE("Octree::childrenExtent(extent, i)") {
    Parfait::Extent<double> extent = {{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree::Node node(extent);
    using namespace Parfait::Octree;
    SECTION("child 0") {
        auto e = childExtent(extent, 0);
        REQUIRE(e.lo[0] == Approx(0));
        REQUIRE(e.lo[1] == Approx(0));
        REQUIRE(e.lo[2] == Approx(0));
        REQUIRE(e.hi[0] == Approx(0.5));
        REQUIRE(e.hi[1] == Approx(0.5));
        REQUIRE(e.hi[2] == Approx(0.5));
    }
    SECTION("child 1") {
        auto e = childExtent(extent, 1);
        REQUIRE(e.lo[0] == Approx(0.5));
        REQUIRE(e.lo[1] == Approx(0));
        REQUIRE(e.lo[2] == Approx(0));
        REQUIRE(e.hi[0] == Approx(1.0));
        REQUIRE(e.hi[1] == Approx(0.5));
        REQUIRE(e.hi[2] == Approx(0.5));
    }
    SECTION("child 2") {
        auto e = childExtent(extent, 2);
        REQUIRE(e.lo[0] == Approx(0.5));
        REQUIRE(e.lo[1] == Approx(0.5));
        REQUIRE(e.lo[2] == Approx(0));
        REQUIRE(e.hi[0] == Approx(1.0));
        REQUIRE(e.hi[1] == Approx(1.0));
        REQUIRE(e.hi[2] == Approx(0.5));
    }
    SECTION("child 3") {
        auto e = childExtent(extent, 3);
        REQUIRE(e.lo[0] == Approx(0));
        REQUIRE(e.lo[1] == Approx(0.5));
        REQUIRE(e.lo[2] == Approx(0));
        REQUIRE(e.hi[0] == Approx(0.5));
        REQUIRE(e.hi[1] == Approx(1.0));
        REQUIRE(e.hi[2] == Approx(0.5));
    }
    SECTION("child 4") {
        auto e = childExtent(extent, 4);
        REQUIRE(e.lo[0] == Approx(0));
        REQUIRE(e.lo[1] == Approx(0));
        REQUIRE(e.lo[2] == Approx(0.5));
        REQUIRE(e.hi[0] == Approx(0.5));
        REQUIRE(e.hi[1] == Approx(0.5));
        REQUIRE(e.hi[2] == Approx(1.0));
    }
    SECTION("child 5") {
        auto e = childExtent(extent, 5);
        REQUIRE(e.lo[0] == Approx(0.5));
        REQUIRE(e.lo[1] == Approx(0));
        REQUIRE(e.lo[2] == Approx(0.5));
        REQUIRE(e.hi[0] == Approx(1.0));
        REQUIRE(e.hi[1] == Approx(0.5));
        REQUIRE(e.hi[2] == Approx(1.0));
    }
    SECTION("child 6") {
        auto e = childExtent(extent, 6);
        REQUIRE(e.lo[0] == Approx(0.5));
        REQUIRE(e.lo[1] == Approx(0.5));
        REQUIRE(e.lo[2] == Approx(0.5));
        REQUIRE(e.hi[0] == Approx(1.0));
        REQUIRE(e.hi[1] == Approx(1.0));
        REQUIRE(e.hi[2] == Approx(1.0));
    }
    SECTION("child 7") {
        auto e = childExtent(extent, 7);
        REQUIRE(e.lo[0] == Approx(0));
        REQUIRE(e.lo[1] == Approx(0.5));
        REQUIRE(e.lo[2] == Approx(0.5));
        REQUIRE(e.hi[0] == Approx(0.5));
        REQUIRE(e.hi[1] == Approx(1.0));
        REQUIRE(e.hi[2] == Approx(1.0));
    }
}

TEST_CASE("DistanceTree Requires domain") {
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree tree(e);
    tree.setMaxDepth(8);
    Parfait::FacetSegment f = {{0, 0, 0}, {1, 0, 0}, {1, 1, 1}};
    tree.insert(&f);

    auto p = tree.closestPoint({10, 10, 10});
    REQUIRE(p[0] == Approx(1.0));
    REQUIRE(p[1] == Approx(1.0));
    REQUIRE(p[2] == Approx(1.0));
}

TEST_CASE("DistanceTree can store multiple facets") {
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree tree(e);
    tree.setMaxDepth(8);
    Parfait::FacetSegment f1 = {{0, 0, 0}, {0.5, 0, 0}, {.5, .5, .5}};
    Parfait::FacetSegment f2 = {{0.5, 0.5, 0.5}, {1, .5, .5}, {1, 1, 1}};
    tree.insert(&f1);
    tree.insert(&f2);

    auto p = tree.closestPoint({10, 10, 10});
    REQUIRE(p[0] == Approx(1.0));
    REQUIRE(p[1] == Approx(1.0));
    REQUIRE(p[2] == Approx(1.0));
}

TEST_CASE("DistanceTree shrink extents") {
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree tree(e);
    tree.setMaxDepth(8);
    Parfait::FacetSegment f1 = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0.0}};
    Parfait::FacetSegment f2 = {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}};
    tree.insert(&f1);
    tree.insert(&f2);

    tree.finalize();

    auto p = tree.closestPoint({10, 10, 10});
    REQUIRE(p[0] == Approx(1));
    REQUIRE(p[1] == Approx(1));
    REQUIRE(p[2] == Approx(1));

    p = tree.closestPoint({0, 0, 0});
    REQUIRE(p[0] == Approx(0));
    REQUIRE(p[1] == Approx(0));
    REQUIRE(p[2] == Approx(0));
}

TEST_CASE("Distance Tree returns facet index") {
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree tree(e);
    tree.setMaxDepth(8);
    Parfait::FacetSegment f1 = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0.0}};
    Parfait::FacetSegment f2 = {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}};
    tree.insert(&f1);
    tree.insert(&f2);
    tree.finalize();
    {
        Parfait::Point<double> p;
        int index;
        std::tie(p, index) = tree.closestPointAndIndex({10, 10, 10});
        REQUIRE(index == 1);
    }
    {
        Parfait::Point<double> p;
        int index;
        std::tie(p, index) = tree.closestPointAndIndex({-1, -1, -1});
        REQUIRE(index == 0);
    }
}

TEST_CASE("Distance Tree can seed a search with an existing known surface location") {
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    Parfait::DistanceTree tree(e);
    tree.setMaxDepth(8);
    Parfait::FacetSegment f1 = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0.0}};
    Parfait::FacetSegment f2 = {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}};
    tree.insert(&f1);
    tree.insert(&f2);
    tree.finalize();

    Parfait::Point<double> known_surface_location = {1.1, 1, 1};
    Parfait::Point<double> query_point = {2, 1, 1};

    SECTION("Can find nearest location as initial known location") {
        auto p = tree.closestPoint(query_point, known_surface_location);
        REQUIRE(p[0] == Approx(known_surface_location[0]));
        REQUIRE(p[1] == Approx(known_surface_location[1]));
        REQUIRE(p[2] == Approx(known_surface_location[2]));
    }

    SECTION("Can get index as flag saying nothing is closer") {
        Parfait::Point<double> p;
        int index;
        std::tie(p, index) = tree.closestPointAndIndex(query_point, known_surface_location);
        REQUIRE(index == -1);
    }
}

TEST_CASE("Distance Tree can fin distances on a xy plane") {
    auto num_points = 1000;
    Parfait::Point<double> center = {0.5, 0.5, 0};
    auto points = Parfait::generateOrderedPointsOnCircleInXYPlane(num_points, center, 0.4);

    Parfait::DistanceTree tree(Parfait::ExtentBuilder::build(points));

    std::vector<Parfait::LineSegment> edges;
    for (int i = 0; i < num_points; i++) {
        int j = (i + 1) % num_points;
        edges.push_back({points[i], points[j]});
    }

    for (auto& e : edges) {
        tree.insert(&e);
    }

    int num_query_points = 10000;
    auto random_points = Parfait::generateRandomPoints(num_query_points, {{0, 0, 0}, {1, 1, 0}});
    std::vector<double> distance(random_points.size());
    for (int i = 0; i < int(random_points.size()); i++) {
        auto q = random_points[i];
        auto p = tree.closestPoint(q);
        distance[i] = p.distance(q);
        p = p - center;
        auto radius = sqrt(p[0] * p[0] + p[1] * p[1]);
        REQUIRE(radius == Approx(0.4).epsilon(1e-1));
    }

    Parfait::PointWriter::write("circle-points-dist-check", random_points, distance);

    auto extents = tree.getLeafExtents();
    Parfait::ExtentWriter::write("quadtree", extents);
}
