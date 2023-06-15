#include <RingAssertions.h>
#include <YogaStatuses.h>
#include <set>

namespace YOGA {
}

TEST_CASE("keep track of state transitions"){
    using namespace YOGA;

    SECTION("Statuses start as unknown and can be set") {
        StatusKeeper status;
        REQUIRE(status.value() == Unknown);
        status.transition(InNode);
        REQUIRE(InNode == status.value());
    }

    // Operations I will need
    //   - transition from one state to the next
    //   - push values to neighbors / check neighbor values
    //   - sync values (regular sync pattern)
    //   - keep history of previous states
    //       - for orphans, dump out state transitions for each one for debugging

    // The idea is that I can swap in this class instead of the vector of integers
    // that I have now, then I can see what's going wrong in a case with orphan points.
    //  Then I will have some flexibility to add constraints at a global level
    //  (e.g., never change a state once it is marked out), instead of repeating the
    //  if(out) check in many places and possibly missing one.
}
