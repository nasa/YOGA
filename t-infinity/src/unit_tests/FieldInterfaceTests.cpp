#include <RingAssertions.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/VectorFieldAdapter.h>

TEST_CASE("Field can generate norms") {
    std::vector<double> v = {1., 2., 3.};
    inf::VectorFieldAdapter field("test", inf::FieldAttributes::Node(), v);

    REQUIRE(3.0 == field.max());
    REQUIRE(6.0 == field.norm(1));
    REQUIRE(std::sqrt(14.) == Approx(field.norm(2)));
    REQUIRE(std::cbrt(36.) == Approx(field.norm(3)));
    REQUIRE(field.max() == Approx(field.norm(11)).epsilon(1.e-2));
}
