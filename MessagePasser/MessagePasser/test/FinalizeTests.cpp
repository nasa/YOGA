#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <functional>

struct ExampleCallback : public MessagePasser::FinalizeCallback {
    explicit ExampleCallback(int n) : n(n) {}
    void eval() override { printf("callback at MPI_Finalize.  Expected: n = 4  Actual: n = %d\n", n); }
    int n;
};

TEST_CASE("Can set function to be called at Finalize") {
    auto atFinalize = []() { return new ExampleCallback(4); };
    REQUIRE_NOTHROW(MessagePasser::setFinalizeCallback(atFinalize));
}