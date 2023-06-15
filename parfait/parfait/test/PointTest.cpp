
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
#include <parfait/Point.h>
using namespace Parfait;

TEST_CASE("Point, Constructor") {
    Point<double> b(1.0, 2.0, 3.0);
    REQUIRE(1.0 == b[0]);
    REQUIRE(2.0 == b[1]);
    REQUIRE(3.0 == b[2]);
}

TEST_CASE("Point < operator") {
    Point<int> a{2, 3, 1};
    Point<int> b{2, 3, 1};
    REQUIRE_FALSE(a < b);
    b = {2, 3, 2};
    REQUIRE(a < b);
    b = {2, 2, 1};
    REQUIRE_FALSE(a < b);
}

TEST_CASE("Point > operator") {
    Point<int> a{2, 3, 1};
    Point<int> b{2, 3, 1};
    REQUIRE_FALSE(a > b);
    b = {2, 3, 2};
    REQUIRE_FALSE(a > b);
    b = {2, 2, 1};
    REQUIRE(a > b);
}

TEST_CASE("Point, DoubleConstructor") {
    double p[3] = {1, 2, 3};
    Point<double> a(p);

    REQUIRE(1.0 == a[0]);
    REQUIRE(2.0 == a[1]);
    REQUIRE(3.0 == a[2]);
}

TEST_CASE("Point, addition") {
    Point<double> a(1, 2, 3);
    Point<double> b = a + a;
    REQUIRE(2 == b[0]);
    REQUIRE(4 == b[1]);
    REQUIRE(6 == b[2]);
}

TEST_CASE("Point, subtraction") {
    Point<double> a(1, 2, 3);
    Point<double> b(3, 2, 1);
    Point<double> c = a - b;

    REQUIRE(-2 == c[0]);
    REQUIRE(0 == c[1]);
    REQUIRE(2 == c[2]);
}

TEST_CASE("Point, scaling") {
    Point<double> a(1, 2, 3);
    Point<double> b = a * 0.5;

    REQUIRE(0.5 == b[0]);
    REQUIRE(1.0 == b[1]);
    REQUIRE(1.5 == b[2]);

    b = 0.5 * a;
    REQUIRE(0.5 == b[0]);
    REQUIRE(1.0 == b[1]);
    REQUIRE(1.5 == b[2]);
}

TEST_CASE("Point, distance") {
    Point<double> a(1, 0, 0);
    Point<double> b(0, 0, 0);

    SECTION("static distance method") {
        double distance = Point<double>::distance(a, b);
        REQUIRE(1 == distance);
    }
    SECTION("member distance method") {
        double distance = a.distance(b);
        REQUIRE(1 == distance);
    }
}

TEST_CASE("Point, magnitude") {
    Point<double> a(4, 0, 0);
    REQUIRE(4.0 == a.magnitude());
}

TEST_CASE("Point, normalization") {
    Point<double> a(3, 4, 5);
    auto root2 = std::sqrt(2);
    Point<double> correct(3.0 * root2 / 10, 2.0 * root2 / 5.0, root2 / 2.0);

    SECTION("New object") {
        auto normalized_a = a.unit();
        REQUIRE(correct[0] == Approx(normalized_a[0]));
        REQUIRE(correct[1] == Approx(normalized_a[1]));
        REQUIRE(correct[2] == Approx(normalized_a[2]));
        REQUIRE(3 == a[0]);
        REQUIRE(4 == a[1]);
        REQUIRE(5 == a[2]);
    }

    SECTION("In-place") {
        a.normalize();
        REQUIRE(correct[0] == Approx(a[0]));
        REQUIRE(correct[1] == Approx(a[1]));
        REQUIRE(correct[2] == Approx(a[2]));
    }
}

TEST_CASE("Point, dot") {
    Point<double> a(1, 1, 1);
    double dot = Point<double>::dot(a, a);
    REQUIRE(3 == dot);
    REQUIRE(3 == a.dot(a));
}

TEST_CASE("Point, scalingInPlace") {
    Point<double> a(1, 2, 3);
    a *= 0.5;
    REQUIRE(0.5 == a[0]);
    REQUIRE(1.0 == a[1]);
    REQUIRE(1.5 == a[2]);
}

TEST_CASE("Point, cross") {
    Point<double> a(1, 0, 0);
    Point<double> b(0, 1, 0);
    Point<double> c(0, 0, 1);
    Point<double> z = Point<double>::cross(a, b);
    REQUIRE(c[0] == Approx(z[0]));
    REQUIRE(c[1] == Approx(z[1]));
    REQUIRE(c[2] == Approx(z[2]));
    REQUIRE(z.approxEqual(a.cross(b)));
}

TEST_CASE("test approxEqual") {
    Point<double> a = {1.000000000000001, 2, 3};
    Point<double> b = {1.0000000000000001, 2, 3};
    REQUIRE(a.approxEqual(b));

    SECTION("default to false if 'inf's are involved") {
        double inf = 1.0 / 0.0;
        b = {inf, inf, inf};
        REQUIRE_FALSE(a.approxEqual(b));
        REQUIRE_FALSE(b.approxEqual(a));
    }
    SECTION("default to false if 'NaN's are involved") {
        double nan = NAN;
        b = {nan, nan, nan};
        REQUIRE_FALSE(a.approxEqual(b));
        REQUIRE_FALSE(b.approxEqual(a));
    }
}

TEST_CASE("Point can be 4D") {
    Parfait::Point<int, 4> p(0, 1, 2, 3);
    REQUIRE(p[0] == 0);
    REQUIRE(p.to_string() == "0 1 2 3");
}

TEST_CASE("Can use format specifier with to_string()") {
    Parfait::Point<double, 4> p(0, 1, 2, 3);
    REQUIRE(p.to_string("%1.2lf") == "0.00 1.00 2.00 3.00");
    REQUIRE(p.to_string("%1.3lf") == "0.000 1.000 2.000 3.000");
    REQUIRE(p.to_string("%1.2e") == "0.00e+00 1.00e+00 2.00e+00 3.00e+00");
}