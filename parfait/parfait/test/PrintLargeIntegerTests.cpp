#include <RingAssertions.h>
#include <parfait/ToString.h>

TEST_CASE("Convert one digit integer to string") {
    REQUIRE(Parfait::bigNumberToStringWithCommas(0) == std::string("0"));
    REQUIRE(Parfait::bigNumberToStringWithCommas(10) == std::string("10"));
    REQUIRE(Parfait::bigNumberToStringWithCommas(100) == std::string("100"));
    REQUIRE(Parfait::bigNumberToStringWithCommas(1000) == std::string("1,000"));
    REQUIRE(Parfait::bigNumberToStringWithCommas(10000) == std::string("10,000"));
}
