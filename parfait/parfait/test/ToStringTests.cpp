#include <RingAssertions.h>
#include <parfait/ToString.h>

TEST_CASE("Convert double to scientific notation") {
    double d = 1.1e-12;
    auto s = Parfait::to_string(d);
    REQUIRE("0.000000" == s);
    s = Parfait::to_string_scientific(d);
    REQUIRE("1.1e-12" == s);
}

TEST_CASE("Convert vector of ints") {
    std::vector<int> v = {1, 2, 7};
    auto s = Parfait::to_string(v);
    REQUIRE("1 2 7 " == s);
}