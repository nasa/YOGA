#include <parfait/Morton.h>
#include <parfait/Extent.h>
#include <RingAssertions.h>
typedef Parfait::Point<double> Point;
typedef Parfait::Extent<double> Extent;
TEST_CASE("MortonId First Cell") {
    unsigned int x = 0, y = 0, z = 0;
    unsigned long int codingID = Parfait::mortonEncode_for(x, y, z);

    REQUIRE(0 == codingID);

    x = 1;
    codingID = Parfait::mortonEncode_magicbits(x, y, z);
    REQUIRE(1 == codingID);

    x = 0;
    y = 1;
    codingID = Parfait::mortonEncode_magicbits(x, y, z);
    REQUIRE(2 == codingID);

    Parfait::DecodeMorton(codingID, x, y, z);
    REQUIRE(0 == x);
    REQUIRE(1 == y);
    REQUIRE(0 == z);
}

TEST_CASE("MortonId Encode") {
    unsigned int x = 1048576;  // (2^21 >> 1)
    unsigned int y = 0;
    unsigned int z = 0;

    unsigned long int mortonID = Parfait::mortonEncode_magicbits(x, y, z);

    REQUIRE(((unsigned long int)1 << 60) == mortonID);
}

TEST_CASE("Morton Decode") {
    unsigned int x, y, z;
    unsigned long int mortonID = 1;
    mortonID = mortonID << 60;
    Parfait::DecodeMorton(mortonID, x, y, z);
    REQUIRE(1048576 == x);
    REQUIRE(0 == y);
    REQUIRE(0 == z);
}

TEST_CASE("Morton Circle") {
    unsigned int x = 0, y = 0, z = 0;
    unsigned long int mortonID;

    unsigned int half = 2097152 >> 1;
    x = half;
    y = 0;
    z = 0;

    mortonID = Parfait::mortonEncode_for(x, y, z);
    Parfait::DecodeMorton(mortonID, x, y, z);
    REQUIRE(half == x);
    REQUIRE(0 == y);
    REQUIRE(0 == z);
}

TEST_CASE("Morton get Point") {
    unsigned int half = 2097152 >> 1;
    unsigned int x = half;
    unsigned int y = 0;
    unsigned int z = 0;
    unsigned long int mortonID = Parfait::mortonEncode_for(x, y, z);

    Extent root_domain(Point(0, 0, 0), Point(1, 1, 1));
    Point p = Parfait::MortonID::getPointFromMortonID(mortonID, root_domain);

    CHECK(p.approxEqual({.5, 0, 0}));

    p = Parfait::MortonID::getPointFromMortonCoords(x, y, z, root_domain);
    CHECK(p.approxEqual({.5, 0, 0}));
}

TEST_CASE("Get Morton from Point") {
    Extent domain({0, 0, 0}, {1, 1, 1});
    unsigned int half = 2097152 >> 1;
    unsigned int x = half;
    unsigned int y = 0;
    unsigned int z = 0;
    unsigned long int expected = Parfait::mortonEncode_for(x, y, z);

    auto id = Parfait::MortonID::getMortonIdFromPoint(domain, {.5, 0, 0});
    REQUIRE(id == expected);
}

TEST_CASE("Morton Curve Spatial Ordering") {
    REQUIRE(Parfait::mortonEncode_magicbits(0, 0, 0) == 0);
    REQUIRE(Parfait::mortonEncode_magicbits(1, 0, 0) == 1);
    REQUIRE(Parfait::mortonEncode_magicbits(0, 1, 0) == 2);
    REQUIRE(Parfait::mortonEncode_magicbits(1, 1, 0) == 3);
    REQUIRE(Parfait::mortonEncode_magicbits(0, 0, 1) == 4);
    REQUIRE(Parfait::mortonEncode_magicbits(1, 0, 1) == 5);
    REQUIRE(Parfait::mortonEncode_magicbits(0, 1, 1) == 6);
    REQUIRE(Parfait::mortonEncode_magicbits(1, 1, 1) == 7);
}
