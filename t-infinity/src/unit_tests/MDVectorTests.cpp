#include <RingAssertions.h>
#include <t-infinity/MDVector.h>

using namespace inf;

TEST_CASE("multi-dimensional vector") {
    SECTION("1D vector") {
        auto vec_1d = MDVector<double, 1>({2});
        REQUIRE(2 == vec_1d.vec.size());
        REQUIRE(2 == vec_1d.indexing(2));
        vec_1d(1) = 2.3;
        REQUIRE(2.3 == vec_1d(1));
    }
    SECTION("2D vector - LayoutRight") {
        auto vec_2d = MDVector<double, 2, LayoutRight>({2, 3});
        REQUIRE(6 == vec_2d.vec.size());
        REQUIRE(0 == vec_2d.indexing(0, 0));
        REQUIRE(1 == vec_2d.indexing(0, 1));
        REQUIRE(2 == vec_2d.indexing(0, 2));
        REQUIRE(3 == vec_2d.indexing(1, 0));
        REQUIRE(4 == vec_2d.indexing(1, 1));
        REQUIRE(5 == vec_2d.indexing(1, 2));
        REQUIRE(2 * 3 - 1 == vec_2d.indexing(1, 2));
        vec_2d(1, 2) = 3.3;
        REQUIRE(3.3 == vec_2d(1, 2));
    }
    SECTION("2D vector - LayoutLeft") {
        auto vec_2d = MDVector<double, 2, LayoutLeft>({2, 3});
        REQUIRE(6 == vec_2d.vec.size());
        REQUIRE(0 == vec_2d.indexing(0, 0));
        REQUIRE(1 == vec_2d.indexing(1, 0));
        REQUIRE(2 == vec_2d.indexing(0, 1));
        REQUIRE(3 == vec_2d.indexing(1, 1));
        REQUIRE(4 == vec_2d.indexing(0, 2));
        REQUIRE(5 == vec_2d.indexing(1, 2));
        REQUIRE(2 * 3 - 1 == vec_2d.indexing(1, 2));
        vec_2d(1, 2) = 3.3;
        REQUIRE(3.3 == vec_2d(1, 2));
    }
    SECTION("3D vector - LayoutRight") {
        auto vec_3d = MDVector<double, 3, LayoutRight>({2, 3, 4});
        REQUIRE(2 * 3 * 4 == vec_3d.vec.size());

        REQUIRE(1 == vec_3d.indexing(0, 0, 1));
        REQUIRE(5 == vec_3d.indexing(0, 1, 1));
        REQUIRE(16 == vec_3d.indexing(1, 1, 0));
        REQUIRE(23 == vec_3d.indexing(1, 2, 3));

        vec_3d(1, 2, 1) = 3.24;
        REQUIRE(3.24 == vec_3d(1, 2, 1));
    }
    SECTION("3D vector - LayoutLeft") {
        auto vec_3d = MDVector<double, 3, LayoutLeft>({2, 3, 4});
        REQUIRE(2 * 3 * 4 == vec_3d.vec.size());

        REQUIRE(1 == vec_3d.indexing(1, 0, 0));
        REQUIRE(3 == vec_3d.indexing(1, 1, 0));
        REQUIRE(23 == vec_3d.indexing(1, 2, 3));

        vec_3d(1, 2, 1) = 3.24;
        REQUIRE(3.24 == vec_3d(1, 2, 1));
    }
    SECTION("4D vector - default (LayoutRight)") {
        auto vec_4d = MDVector<double, 4>({5, 4, 3, 2});
        REQUIRE(2 * 3 * 4 * 5 == vec_4d.vec.size());

        REQUIRE(1 == vec_4d.indexing(0, 0, 0, 1));
        REQUIRE(3 == vec_4d.indexing(0, 0, 1, 1));
        REQUIRE(9 == vec_4d.indexing(0, 1, 1, 1));
        REQUIRE(119 == vec_4d.indexing(4, 3, 2, 1));

        vec_4d(3, 2, 1, 1) = 4.24;
        REQUIRE(4.24 == vec_4d(3, 2, 1, 1));
    }
}

TEST_CASE("multi-dimensional vector copy ctor") {
    auto vec = Vector3D<double>({2, 2, 2});
    vec(0, 0, 0) = 1.2;
    auto vec2 = vec;
    REQUIRE(1.2 == vec2(0, 0, 0));
}
TEST_CASE("multi-dimensional vector initialization") {
    auto vec = Vector3D<int>({2, 2, 2}, -1);
    for (int v : vec.vec) REQUIRE(v == -1);
}
