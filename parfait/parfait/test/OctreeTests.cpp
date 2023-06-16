#include <parfait/Facet.h>
#include <parfait/OctreeStorage.h>
#include <RingAssertions.h>

TEST_CASE("Octree must be initialized") {
    Parfait::OctreeStorage<Parfait::Facet> tree;
    Parfait::Facet f = {{0, 0, 0}, {1, 1, 1}, {1, 0, 1}};
    SECTION("Throws if root extent not set") { REQUIRE_THROWS(tree.insert(&f)); }
}
