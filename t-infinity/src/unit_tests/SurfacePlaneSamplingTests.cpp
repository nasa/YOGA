#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/SurfacePlaneSampling.h>
#include "MockFields.h"
#include "MockMeshes.h"

TEST_CASE("Can take a mesh and fields, and sample onto a new mesh and new fields"){
    // -- Get your mesh and your fields
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() != 1) return;
    auto filename = std::string(GRIDS_DIR) + "lb8.ugrid/10x10x10_regular.lb8.ugrid";
    auto mesh = inf::shortcut::loadMesh(mp, filename);
    auto function = [](double x, double y, double z) {
        double pi = 4.0*atan(1.0);
        return 3 * x + 4 * sin(2*pi*y) - cos(2*pi*z);
    };
    auto field = inf::createNodeField(inf::test::fillFieldAtNodes(*mesh, function));


    // -- Choose your surface tags and where to cut.
    std::set<int> tags = {3};
    Parfait::Plane<double> plane({1.0, 0, 0}, {0.5, 0, 0});

    // -- get a point cloud back on the surface as a mesh, and all the fields you wanted sampled.
    auto mesh_and_fields = inf::surfacePlaneSample(mp, mesh, {field}, plane, tags);

    // -- visualize with the .csv writer.
    inf::shortcut::visualize("test.dat", mp, mesh_and_fields.mesh, mesh_and_fields.fields);
}

TEST_CASE("Can cut a single hex") {
    auto mesh = std::make_shared<inf::TinfMesh>(inf::mock::oneQuad(), 0);

    Parfait::Plane<double> plane({1.0, 0, 0}, {0.5, 0, 0});
    auto cut_template = inf::EdgeCutTemplate::cutSurfaceWithPlane(*mesh, plane, {0});
    REQUIRE(cut_template.numCutEdges() == 2);
    auto points = cut_template.getCutPoints(*mesh);
    REQUIRE(points[0][0] == Approx(0.5));
    REQUIRE(points[1][0] == Approx(0.5));
}

TEST_CASE("Can sample a field on a surface") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto filename = std::string(GRIDS_DIR) + "lb8.ugrid/10x10x10_regular.lb8.ugrid";
    std::set<int> tags = {3};
    auto mesh = inf::shortcut::loadMesh(mp, filename);
    auto function = [](double x, double y, double z) { return 3 * x + 4 * y - z; };
    auto field = inf::createNodeField(inf::test::fillFieldAtNodes(*mesh, function));

    Parfait::Plane<double> plane({1.0, 0, 0}, {0.5, 0, 0});
    auto cut_template = inf::EdgeCutTemplate::cutSurfaceWithPlane(*mesh, plane, {3});

    auto get_field_at_node = [&](int n) {
        double d;
        field->value(n, &d);
        return d;
    };
    auto cut_field = cut_template.apply(get_field_at_node);
    auto points = cut_template.getCutPoints(*mesh);
    REQUIRE(points.size() == cut_field.size());

    for (int i = 0; i < int(points.size()); i++) {
        auto p = points[i];
        double f = cut_field[i];
        REQUIRE(function(p[0], p[1], p[2]) == Approx(f));
    }
}
