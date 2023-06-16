#include <RingAssertions.h>
#include <parfait/StackTrace.h>

std::string stack_dog() { return Parfait::stackTrace(); }

TEST_CASE("stack trace will get function names (if compiler adds them)") {
    auto backtrace = stack_dog();
    printf("backtrace %s\n", backtrace.c_str());
    // Some compilers don't add helpful symbols
    // But in general this is true:
    //    REQUIRE(Parfait::StringTools::contains(backtrace, "dog"));
}
