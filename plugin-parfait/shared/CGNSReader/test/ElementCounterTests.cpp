#include <cgnslib.h>
#include <RingAssertions.h>
#include <ElementCounter.h>

TEST_CASE("ElementCounter") {
    ElementCounter counter;
    long tet_count = counter.currentCount(CGNS_ENUMT(TETRA_4));
    REQUIRE(tet_count == 0);
    counter.setCounter(CGNS_ENUMT(TETRA_4), 100);
    REQUIRE(100 == counter.currentCount(CGNS_ENUMT(TETRA_4)));
    REQUIRE(0 == counter.currentCount(CGNS_ENUMT(TETRA_16)));
}