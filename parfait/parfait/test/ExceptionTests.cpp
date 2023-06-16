#include <RingAssertions.h>
#include <parfait/Throw.h>
#include <parfait/StringTools.h>

void dog() { throw Parfait::Exception(); }

TEST_CASE("exception exists") {
    bool caught = false;
    std::string backtrace;
    try {
        dog();
    } catch (const Parfait::Exception& e) {
        caught = true;
        backtrace = e.stack_trace();
    }
    REQUIRE(caught);
    printf("backtrace %s\n", backtrace.c_str());
    // Some compilers don't add helpful symbols
    // But in general this is true:
    //    REQUIRE(Parfait::StringTools::contains(backtrace, "dog"));
}
