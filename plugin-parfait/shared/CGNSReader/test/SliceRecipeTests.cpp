#include <RingAssertions.h>
#include <vector>
#include <SliceAcrossSections.h>

TEST_CASE("Get list of sections to achieve range") {
    std::vector<long> count_per_section = {344, 200, 200};
    std::vector<SliceAcrossSections::Slice> slices =
        SliceAcrossSections::calcSlicesForRange(count_per_section, 100, 500);
    REQUIRE(slices.size() == 2);
    REQUIRE(slices[0].section == 0);
    REQUIRE(slices[0].start == 100);
    REQUIRE(slices[0].end == 344);

    REQUIRE(slices[1].section == 1);
    REQUIRE(slices[1].start == 0);
    REQUIRE(slices[1].end == 156);
}

TEST_CASE("get slice in range") {
    using namespace SliceAcrossSections;
    Slice s = getRangeInSection(0, 344, 0, 344);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 344);

    s = getRangeInSection(50, 100, 0, 344);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 50);

    s = getRangeInSection(50, 100, 55, 344);
    REQUIRE(s.start == 5);
    REQUIRE(s.end == 50);

    s = getRangeInSection(50, 60, 55, 60);
    REQUIRE(s.start == 5);
    REQUIRE(s.end == 10);

    s = getRangeInSection(50, 51, 50, 60);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 1);

    s = getRangeInSection(50, 55, 0, 65);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 5);

    s = getRangeInSection(50, 55, 0, 5);
    REQUIRE(s.start == -1);
    REQUIRE(s.end == -1);

    s = getRangeInSection(50, 55, 55, 60);
    REQUIRE(s.start == -1);
    REQUIRE(s.end == -1);

    //-- checking corners
    s = getRangeInSection(3, 8, 0, 8);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 5);

    s = getRangeInSection(3, 8, 3, 8);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 5);

    s = getRangeInSection(3, 8, 0, 3);
    REQUIRE(s.start == -1);
    REQUIRE(s.end == -1);

    s = getRangeInSection(3, 8, 8, 12);
    REQUIRE(s.start == -1);
    REQUIRE(s.end == -1);

    s = getRangeInSection(344, 544, 100, 500);
    REQUIRE(s.start == 0);
    REQUIRE(s.end == 156);
}