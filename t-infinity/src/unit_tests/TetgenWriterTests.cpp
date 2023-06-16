#include <RingAssertions.h>
#include <parfait/STLFactory.h>
#include <parfait/Flatten.h>
#include <t-infinity/STLConverters.h>
#include <t-infinity/TetGenWriter.h>

TEST_CASE("tetgen from surface mesh and hole points") {
    auto outer_sphere = Parfait::STLFactory::generateSphere({0, 0, 0}, 10);
    auto inner_sphere = Parfait::STLFactory::generateSphere({0, 0, 0}, 1);
    std::vector<Parfait::Facet> facets =
        Parfait::flatten(std::vector<std::vector<Parfait::Facet>>{outer_sphere, inner_sphere});
    auto mesh = inf::STLConverters::stitchSTLToInfinity(facets);
    Parfait::Point<double> hole_point = {0, 0, 0};

    inf::writeTetGenPoly("sphere-in-sphere", *mesh, {hole_point});

    REQUIRE(Parfait::FileTools::doesFileExist("sphere-in-sphere.poly"));
}