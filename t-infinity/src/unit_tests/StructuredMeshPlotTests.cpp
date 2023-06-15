#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/StructuredMeshPlot.h>
#include <t-infinity/SMesh.h>

TEST_CASE("Structured mesh plot") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;

    double lo_x = mp.Rank();
    double hi_x = mp.Rank() + 1;

    std::array<int, 3> cell_dims = {10, 10, 10};
    std::array<int, 3> node_dims = {cell_dims[0] + 1, cell_dims[1] + 1, cell_dims[2] + 1};
    Parfait::Point<double> block_lo = {lo_x, 0, 0};
    Parfait::Point<double> block_hi = {hi_x, 1, 1};
    auto block = std::make_shared<inf::CartesianBlock>(block_lo, block_hi, cell_dims, mp.Rank());

    auto mesh = std::make_shared<inf::SMesh>();
    mesh->add(block);

    std::string plot3d_filename = "test-structured-mesh-out";
    inf::StructuredPlotter plotter(mp, plot3d_filename, mesh);

    auto field = std::make_shared<inf::SField>("first-field", inf::FieldAttributes::Node());
    auto field2 = std::make_shared<inf::SField>("second-field", inf::FieldAttributes::Node());
    auto field_block1 = field->createBlock(block->nodeDimensions(), block->blockId());
    auto field_block2 = field2->createBlock(block->nodeDimensions(), block->blockId());
    for (int i = 0; i < node_dims[0]; i++) {
        for (int j = 0; j < node_dims[1]; j++) {
            for (int k = 0; k < node_dims[2]; k++) {
                auto p = block->point(i, j, k);
                field_block1->values(i, j, k) = p[0] * 3.3 + p[1] * 2.0 + p[2] * -1;
                field_block2->values(i, j, k) = p[0] * 0.1 + p[1] * 0.2 + p[2] * 0.3;
            }
        }
    }
    field->add(field_block1);
    plotter.addField(field);

    field2->add(field_block2);
    plotter.addField(field2);

    REQUIRE_NOTHROW(plotter.visualize());
    if (mp.Rank() == 0) {
        REQUIRE(0 == remove((plot3d_filename + ".g").c_str()));
        REQUIRE(0 == remove((plot3d_filename + ".q").c_str()));
        REQUIRE(0 == remove((plot3d_filename + ".nam").c_str()));
    }
}
