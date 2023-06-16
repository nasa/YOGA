#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Pipeline.h>

TEST_CASE("Pipelines exist") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    inf::MeshAndFields mesh_and_fields;
    mesh_and_fields.mesh = inf::CartMesh::create(mp, 5, 5, 5);

    inf::Pipeline pipeline;

    auto statistics_printer = std::make_shared<inf::MeshStatisticsPrinter>();

    auto sphere_selector = std::make_shared<inf::SphereSelector>(0.5, Parfait::Point<double>{0, 0, 0});

    pipeline.append(statistics_printer);
    pipeline.append(std::make_shared<inf::CellSelectorTransform>(sphere_selector));
    mesh_and_fields = pipeline.apply(mp, mesh_and_fields);

    //    inf::shortcut::visualize("sphere-box", mp, mesh_and_fields.mesh, mesh_and_fields.fields);
}
