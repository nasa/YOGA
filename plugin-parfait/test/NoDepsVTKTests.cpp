#include <Viz/ParfaitViz.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <RingAssertions.h>
#include <string>
#include "../NodeCenteredPreProcessor/NC_PreProcessor.h"

TEST_CASE("No Deps vtk") {
    std::string six_cell_mesh = std::string(GRIDS_DIR) + "lb8.ugrid/20x20_quarter_cylinder_tets.lb8.ugrid";
    NC_PreProcessor preProcessor;
    MessagePasser mp(MPI_COMM_WORLD);
    auto mesh = preProcessor.load(mp.getCommunicator(), six_cell_mesh);

    std::vector<double> cell_data(mesh->cellCount());
    std::vector<int> temp(8);
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->ownedCell(c)) {
            mesh->cell(c, temp.data());
            auto p = mesh->node(temp[0]);
            cell_data[c] = double(p[0]);
        }
    }
    std::vector<double> node_data(mesh->nodeCount());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        if (mesh->ownedNode(n)) {
            auto p = mesh->node(n);
            node_data[n] = p[0];
        }
    }

    auto node_field =
        std::make_shared<inf::VectorFieldAdapter>("node-field", inf::FieldAttributes::Node(), 1, node_data);
    auto cell_field =
        std::make_shared<inf::VectorFieldAdapter>("cell-field", inf::FieldAttributes::Cell(), 1, cell_data);
    ParfaitViz no_dep_vtk("output-no-deps", mesh, MPI_COMM_WORLD);
    no_dep_vtk.addField(cell_field);
    no_dep_vtk.addField(node_field);
    no_dep_vtk.visualize();
}
