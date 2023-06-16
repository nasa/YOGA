#include <RingAssertions.h>
#include <parfait/FloatingPointExceptions.h>

TEST_CASE("divide by zero without exiting") {
    // The code was implemented via TDD with this test,
    // but it has to be turned off, because toggling FPE
    // breaks the ASAN build (which doesn't allow FPE)

    // Parfait::enableFloatingPointExceptions();
    // Parfait::disableFloatingPointExceptions();
    // double x = 1.0 / 0.0;
    // REQUIRE(std::isinf(x));
}