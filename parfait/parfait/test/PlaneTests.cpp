
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
#include <parfait/Plane.h>

typedef Parfait::Plane<double> Plane;

TEST_CASE("Can reflect Parfait::Point about YZ-plane") {
    auto point = Parfait::Point<double>(1, 2, 3);
    SECTION("manually create plane-normal vector") {
        auto plane = Plane({1, 0, 0}, {0, 0, 0});
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == -1.0);
        REQUIRE(reflected_point[1] == 2.0);
        REQUIRE(reflected_point[2] == 3.0);
    }

    SECTION("use enum") {
        auto plane = Plane(Plane::YZ);
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == -1.0);
        REQUIRE(reflected_point[1] == 2.0);
        REQUIRE(reflected_point[2] == 3.0);
    }
}

TEST_CASE("Can reflect Parfait::Point about XZ-plane") {
    auto point = Parfait::Point<double>(1, 2, 3);
    SECTION("manual plane-normal") {
        auto plane = Plane({0, 1, 0}, {0, 0, 0});
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == 1.0);
        REQUIRE(reflected_point[1] == -2.0);
        REQUIRE(reflected_point[2] == 3.0);
    }
    SECTION("use enum") {
        auto plane = Plane(Plane::XZ);
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == 1.0);
        REQUIRE(reflected_point[1] == -2.0);
        REQUIRE(reflected_point[2] == 3.0);
    }
}

TEST_CASE("Can reflect Parfait::Point about XY-plane") {
    auto point = Parfait::Point<double>(1, 2, 3);
    SECTION("manually specify plane normal") {
        auto plane = Plane({0, 0, 1}, {0, 0, 0});
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == 1.0);
        REQUIRE(reflected_point[1] == 2.0);
        REQUIRE(reflected_point[2] == -3.0);
    }
    SECTION("use enum") {
        auto plane = Plane(Plane::XY);
        auto reflected_point = plane.reflect(point);
        REQUIRE(reflected_point[0] == 1.0);
        REQUIRE(reflected_point[1] == 2.0);
        REQUIRE(reflected_point[2] == -3.0);
    }
}

TEST_CASE("Can intersect an edge with a plane") {
    Parfait::Point<double> normal = {1, 0, 0};
    Parfait::Point<double> origin = {0.5, 0, 0};
    auto plane = Plane(normal, origin);

    REQUIRE(plane.edgeIntersectionWeight({0, 0, 0}, {1, 0, 0}) == 0.5);
    REQUIRE(plane.edgeIntersectionWeight({0, 0, 0}, {2, 0, 0}) == 0.75);
    REQUIRE(plane.edgeIntersectionWeight({0, 0, 0}, {1.5, 0, 0}) == Approx(1.0 - 1.0 / 3.0));
}

TEST_CASE("Can create orthogonal vectors in the plane") {
    SECTION("If a cartesian aligned select cartesian unit vectors x normal") {
        Parfait::Point<double> normal = {1, 0, 0};
        auto plane = Parfait::Plane<double>(normal);
        Parfait::Point<double> v1, v2;
        std::tie(v1, v2) = plane.orthogonalVectors();

        REQUIRE(v1.approxEqual({0, 1, 0}));
        REQUIRE(v2.approxEqual({0, 0, 1}));
    }
    SECTION("If a cartesian aligned select cartesian unit vectors x normal") {
        Parfait::Point<double> normal = {0, 1, 0};
        auto plane = Parfait::Plane<double>(normal);
        Parfait::Point<double> v1, v2;
        std::tie(v1, v2) = plane.orthogonalVectors();

        REQUIRE(v1.approxEqual({1, 0, 0}));
        REQUIRE(v2.approxEqual({0, 0, 1}));
    }
    SECTION("If a cartesian aligned select cartesian unit vectors x normal") {
        Parfait::Point<double> normal = {0, 0, 1};
        auto plane = Parfait::Plane<double>(normal);
        Parfait::Point<double> v1, v2;
        std::tie(v1, v2) = plane.orthogonalVectors();

        REQUIRE(v1.approxEqual({1, 0, 0}));
        REQUIRE(v2.approxEqual({0, 1, 0}));
    }

    SECTION("Random orientation plane creates orthogonal vectors") {
        Parfait::Point<double> normal = {1, 1, 0};
        normal.normalize();
        auto plane = Parfait::Plane<double>(normal);
        Parfait::Point<double> v1, v2;
        std::tie(v1, v2) = plane.orthogonalVectors();
        REQUIRE(Parfait::Point<double>::dot(v1, v2) == Approx(0.0).margin(1.0e-10));
        auto v3 = Parfait::Point<double>::cross(v1, v2);
        printf("normal = %s\n", normal.to_string().c_str());
        printf("    v1 = %s\n", v1.to_string().c_str());
        printf("    v2 = %s\n", v2.to_string().c_str());
        printf("    v3 = %s\n", v3.to_string().c_str());
        REQUIRE(v3.approxEqual(normal));
    }
}