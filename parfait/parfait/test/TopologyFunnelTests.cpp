#include <RingAssertions.h>
#include <parfait/Topology.h>

TEST_CASE("Topology funnel") {
    SECTION("Throw if do own doesn't match gids") { REQUIRE_THROWS(Parfait::Topology({0, 4, 5}, {false, false})); }

    SECTION("Throw if two owned local ids map to the same GID") {
        REQUIRE_THROWS(Parfait::Topology({0, 4, 4}, {true, true, true}));
    }

    SECTION("But two non-owned LIDS mapping to the same GID is fine.") {
        REQUIRE_NOTHROW(Parfait::Topology({0, 4, 4}, {true, false, false}));
    }

    SECTION("Build g2l for all owned entries") {
        Parfait::Topology top({0, 4, 6}, {true, true, true});
        REQUIRE(top.global_to_local.size() == 3);
    }
}
