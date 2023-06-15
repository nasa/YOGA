#include <RingAssertions.h>
#include <memory>
#include <algorithm>
#include <queue>
#include <t-infinity/MeshBuilder.h>
#include <parfait/PointGenerator.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>
#include <parfait/Linspace.h>
#include <t-infinity/Shortcuts.h>
#include "MockMeshes.h"
#include "t-infinity/MeshHelpers.h"
#include "parfait/PointWriter.h"
#include "parfait/VectorTools.h"
#include "parfait/Checkpoint.h"
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/Geometry.h>
#include <t-infinity/MeshQualityMetrics.h>
#include <t-infinity/MeshMechanics.h>
#include "AdaptationTestFixtures.h"

using namespace inf;

TEST_CASE("Can construct an empty mesh", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto builder = inf::experimental::MeshBuilder(mp);
    REQUIRE(builder.mesh->cellCount() == 0);
    REQUIRE(builder.mesh->nodeCount() == 0);
}

TEST_CASE("Mesh builder test fixture works", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto cart_mesh = inf::CartMesh::create(mp, 10, 10, 10);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    REQUIRE(cart_mesh == nullptr);

    REQUIRE(builder.mesh->nodeCount() > 0);
    REQUIRE(builder.mesh->cellCount() > 0);
}

TEST_CASE("If you add a NODE it must be marked as not modifiable") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    inf::experimental::MeshBuilder builder(mp);
    builder.addNode({1,1,1});
    builder.addCell(inf::MeshInterface::NODE, {0});
    REQUIRE(not builder.isNodeModifiable(0));
}

TEST_CASE("Mesh Builder can delete more than it creates in parallel", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    int num_nodes = mesh->nodeCount();
    int num_cells = mesh->cellCount();
    auto builder = inf::experimental::MeshBuilder(mp, std::move(mesh));
    if (mp.Rank() == 0) {
        REQUIRE(mp.Rank() == builder.mesh->nodeOwner(0));
        auto n2n = inf::NodeToNode::build(*builder.mesh);
        for (auto& neighbor : n2n[0]) {
            REQUIRE(mp.Rank() == builder.mesh->nodeOwner(neighbor));
        }
        auto neighbors = builder.node_to_cells.at(0);
        for (auto& neighbor : neighbors) {
            builder.deleteCell(neighbor.type, neighbor.index);
        }
        builder.deleteNode(0);
    }
    builder.sync();

    if (mp.Rank() == 0) {
        REQUIRE(builder.mesh->nodeCount() == num_nodes - 1);
        REQUIRE(builder.mesh->cellCount() == num_cells - 4);  // one hex and 3 quad faces
    } else {
        REQUIRE(builder.mesh->nodeCount() == num_nodes);  // nothing deleted
        REQUIRE(builder.mesh->cellCount() == num_cells);
    }
    // ensure all the gids are correct, as is needed by the visualization code.
    inf::shortcut::visualize("this.call.is.intended", mp, builder.mesh, {});
}

TEST_CASE("Mesh Builder can delete more than it creates in serial", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = std::make_shared<inf::TinfMesh>(inf::mock::twoTouchingTets(), 0);
    int num_nodes = mesh->nodeCount();
    int num_cells = mesh->cellCount();
    auto builder = inf::experimental::MeshBuilder(mp, std::move(mesh));
    inf::shortcut::visualize("build.0", mp, builder.mesh, {});
    auto neighbors = builder.node_to_cells.at(0);
    for (auto& neighbor : neighbors) {
        builder.deleteCell(neighbor.type, neighbor.index);
    }
    builder.deleteNode(0);
    builder.sync();

    std::set<long> nodal_gids;
    for (int n = 0; n < builder.mesh->nodeCount(); n++) {
        nodal_gids.insert(builder.mesh->globalNodeId(n));
    }

    REQUIRE(nodal_gids == std::set<long>{0, 1, 2, 3});

    printf("-----\n\n");
    builder.printNodes();
    builder.printCells();
    builder.printNodeToCells();

    inf::shortcut::visualize("build.1", mp, builder.mesh, {});

    REQUIRE(builder.mesh->nodeCount() == num_nodes - 1);
    REQUIRE(builder.mesh->cellCount() == num_cells - 1);
}

template <typename C2C>
bool hasCellSupport(int c, const C2C& c2c, const inf::MeshInterface& mesh) {
    int owner = mesh.cellOwner(c);
    if (owner != mesh.partitionId()) return false;
    for (auto& neighbor : c2c[c]) {
        if (mesh.cellOwner(neighbor) != owner) return false;
    }
    return true;
}
TEST_CASE("Don't rebuild lazy deleted cells with other deleted cells", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create(mp, 3, 3, 3);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    int num_hexs = builder.mesh->cellCount(inf::MeshInterface::HEXA_8);
    builder.deleteCell(inf::MeshInterface::HEXA_8, 0);
    builder.deleteCell(inf::MeshInterface::HEXA_8, num_hexs - 1);

    builder.rebuildLocal();
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::HEXA_8) == num_hexs - 2);
    REQUIRE(builder.mesh->mesh.cells[inf::MeshInterface::HEXA_8][0] != -1);
}

TEST_CASE("Can find appropriate ids to copy from") {
    std::vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::set<int> to_delete = {0, 4, 5, 6, 9};
    int from = 9;
    for (auto id : to_delete) {
        from = inf::experimental::MeshBuilder::findLastNotDeletedEntry(to_delete, from);
        if (from >= id) {
            data[id] = data[from];
        }
        data.pop_back();
        from--;
    }

    REQUIRE(data == std::vector<int>{8, 1, 2, 3, 7});
}

TEST_CASE("Can ask if a node or cell is modifiable in parallel", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto cart_mesh = inf::CartMesh::create(mp, 3, 3, 3);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    auto n2n = inf::NodeToNode::buildForAnyNodeInSharedCell(*builder.mesh);
    auto c2c = inf::CellToCell::build(*builder.mesh);
    for (int c = 0; c < builder.mesh->cellCount(); c++) {
        if (hasCellSupport(c, c2c, *builder.mesh)) {
            auto cell = builder.mesh->cellIdToTypeAndLocalId(c);
            REQUIRE(builder.isCellModifiable({cell.first, cell.second}));
        } else {
            auto cell = builder.mesh->cellIdToTypeAndLocalId(c);
            REQUIRE_FALSE(builder.isCellModifiable({cell.first, cell.second}));
        }
    }
}

TEST_CASE("Can coarsen a bunch", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto cart_mesh = inf::CartMesh::create(mp, 3, 3, 3);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    for (int n = 0; n < builder.mesh->nodeCount(); n++) {
        if (random() % 100 < 25) {
            if (builder.isNodeModifiable(n)) {
                auto cell_neighbors = builder.node_to_cells[n];
                for (auto& neighbor : cell_neighbors) {
                    builder.deleteCell(neighbor.type, neighbor.index);
                }
                builder.deleteNode(n);
            }
        }
    }
    builder.sync();
    //    builder.printNodeToCells();
    builder.printCells();
    inf::shortcut::visualize("builder.random", mp, builder.mesh);
}

bool noNodesAboveNodeCount(const inf::MeshInterface& mesh) {
    int node_count = mesh.nodeCount();
    std::vector<int> cell;
    for (int c = 0; c < mesh.cellCount(); c++) {
        mesh.cell(c, cell);
        for (int i = 0; i < int(cell.size()); i++) {
            if (cell[i] >= node_count) {
                printf("Cell %d, type %s has node %d above node count %d\n",
                       c,
                       inf::MeshInterface::cellTypeString(mesh.cellType(c)).c_str(),
                       cell[i],
                       node_count);
                return false;
            }
        }
    }
    return true;
}

TEST_CASE("Mesh builder can delete a node including renumbering cells after you shuffle a node when compacting",
          "[builder]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create(mp, 2, 2, 2);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    int orig_node_count = builder.mesh->nodeCount();
    auto n2c = inf::NodeToCell::build(*builder.mesh);
    for (int c : n2c[0]) {
        auto type_and_index = builder.mesh->cell_id_to_type_and_local_id.at(c);
        builder.deleteCell(type_and_index.first, type_and_index.second);
    }
    builder.deleteNode(0);
    builder.rebuildLocal();
    REQUIRE(builder.mesh->nodeCount() == orig_node_count - 1);
    REQUIRE(noNodesAboveNodeCount(*builder.mesh));
}

TEST_CASE("Mesh builder can create more cells in parallel", "[builder]") {
    using CellType = inf::MeshInterface::CellType;
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto cart_mesh = inf::CartMesh::create(mp, 4, 4, 4);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    long global_node_count_before = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_before = inf::globalCellCount(mp, *builder.mesh);

    CellType type;
    int type_index;
    int inf_cell_id = -1;
    if (mp.Rank() == 0) {
        for (int cell_id = 0; cell_id < builder.mesh->cellCount(); cell_id++) {
            if (builder.mesh->cellOwner(cell_id) == mp.Rank()) {
                std::tie(type, type_index) = builder.mesh->cellIdToTypeAndLocalId(cell_id);
                if (type == CellType::HEXA_8 and builder.isCellModifiable({type, type_index})) {
                    inf_cell_id = cell_id;
                    break;
                }
            }
        }
        PARFAIT_ASSERT(inf_cell_id >= 0, "Could not find any modifiable hex");
        std::vector<int> cell_nodes(8);
        inf::Cell hex(*builder.mesh, inf_cell_id);
        std::vector<std::vector<int>> hex_faces;
        for (int i = 0; i < 6; i++) {
            auto face_nodes = hex.faceNodes(i);
            std::reverse(face_nodes.begin(), face_nodes.end());
            hex_faces.push_back(face_nodes);
        }

        builder.deleteCell(CellType::HEXA_8, 0);
        builder.steinerCavity(hex_faces);
    }
    builder.sync();

    long global_node_count_after = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_after = inf::globalCellCount(mp, *builder.mesh);

    REQUIRE(global_node_count_after == global_node_count_before + 1);
    REQUIRE(global_cell_count_after == global_cell_count_before + 6 - 1);
}

TEST_CASE("Mesh builder can add multiple nodes simultaneously", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto cart_mesh = inf::CartMesh::create(mp, 3, 3, 3);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    long global_node_count_before = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_before = inf::globalCellCount(mp, *builder.mesh);

    builder.addNode({100.0 * mp.Rank(), 10.0 * mp.Rank(), -10.0 * mp.Rank()});
    builder.sync();

    long global_node_count_after = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_after = inf::globalCellCount(mp, *builder.mesh);

    REQUIRE(global_node_count_after == global_node_count_before + mp.NumberOfProcesses());
    REQUIRE(global_cell_count_after == global_cell_count_before);
}

TEST_CASE("Mesh builder can add multiple cells simultaneously in parallel", "[builder]") {
    using CellType = inf::MeshInterface::CellType;
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto cart_mesh = inf::CartMesh::create(mp, 3, 3, 3);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    long global_node_count_before = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_before = inf::globalCellCount(mp, *builder.mesh);

    int node_a = builder.addNode({100.0 * mp.Rank(), 10.0 * mp.Rank(), -10.0 * mp.Rank()});
    int node_b = builder.addNode({100.0 * mp.Rank(), 10.0 * mp.Rank(), -10.0 * mp.Rank()});
    int node_c = builder.addNode({100.0 * mp.Rank(), 10.0 * mp.Rank(), -10.0 * mp.Rank()});

    builder.addCell(CellType::TRI_3, {node_a, node_b, node_c});
    builder.sync();

    long global_node_count_after = inf::globalNodeCount(mp, *builder.mesh);
    long global_cell_count_after = inf::globalCellCount(mp, *builder.mesh);

    REQUIRE(global_node_count_after == global_node_count_before + mp.NumberOfProcesses() * 3);
    REQUIRE(global_cell_count_after == global_cell_count_before + mp.NumberOfProcesses());
}

TEST_CASE("Mesh builder can shuffle cells that are not modifiable", "[builder]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto cart_mesh = inf::CartMesh::create(mp, 8, 8, 8);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    int num_nodes_modifiable = 0;
    for (int n = 0; n < builder.mesh->nodeCount(); n++) {
        if (builder.isNodeModifiable(n)) num_nodes_modifiable++;
    }

    int num_cells_modifiable = 0;
    for (auto& pair : builder.mesh->mesh.cell_tags) {
        auto type = pair.first;
        for (int c = 0; c < int(pair.second.size()); c++) {
            if (builder.isCellModifiable({type, c})) {
                num_cells_modifiable++;
            }
        }
    }
    printf("rank %d has %d modifiable cells\n", mp.Rank(), num_cells_modifiable);

    int num_cells_deleted = 0;

    for (int n = 0; n < builder.mesh->nodeCount(); n++) {
        if (builder.isNodeModifiable(n)) {
            auto neighbors = builder.node_to_cells.at(n);
            for (auto cell : neighbors) {
                builder.deleteCell(cell.type, cell.index);
                num_cells_deleted++;
            }
            builder.deleteNode(n);
            break;
        }
    }

    builder.rebuildLocal();

    int now_num_nodes_modifiable = 0;
    for (int n = 0; n < builder.mesh->nodeCount(); n++) {
        if (builder.isNodeModifiable(n)) now_num_nodes_modifiable++;
    }

    int now_num_cells_modifiable = 0;
    for (auto& pair : builder.mesh->mesh.cell_tags) {
        auto type = pair.first;
        for (int c = 0; c < int(pair.second.size()); c++) {
            if (builder.isCellModifiable({type, c})) {
                now_num_cells_modifiable++;
            }
        }
    }

    REQUIRE(now_num_nodes_modifiable == num_nodes_modifiable - 1);
    REQUIRE(now_num_cells_modifiable == num_cells_modifiable - num_cells_deleted);
}

TEST_CASE("Mesh builder identify cavity from list of nodes") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::createSurface(mp, 8, 8, 8);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    std::set<int> delete_nodes = {36};

    auto cavity = builder.findCavity(delete_nodes);
    REQUIRE(cavity.exposed_faces.size() == 8);
    REQUIRE(cavity.cells.size() == 4);
}

TEST_CASE("Mesh builder edge collapse") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::createSurface(mp, 8, 8, 8);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    int orig_node_count = builder.mesh->nodeCount();
    std::set<int> delete_nodes = {112, 121};
    builder.edgeCollapse(112, 121);
    builder.sync();
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::TRI_3) == 12);
    REQUIRE(builder.mesh->nodeCount() == orig_node_count - 1);
}

TEST_CASE("Mesh builder edge split") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::createSurface(mp, 8, 8, 8);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    int orig_node_count = builder.mesh->nodeCount();
    builder.surfaceEdgeSplit(112, 121);
    builder.sync();
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::TRI_3) == 6);
    REQUIRE(builder.mesh->nodeCount() == orig_node_count + 1);
    //    inf::shortcut::visualize("edge-split", mp, builder.mesh);
}

TEST_CASE("Find starting cell from point") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create(mp, 8, 8, 8);
    int original_node_count = cart_mesh->nodeCount();
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    auto p = Parfait::Point<double>{0.25, 0.25, 0.25};
    auto cell = builder.findHostCellId(p, {inf::MeshInterface::HEXA_8, 0});
    REQUIRE(cell.type == inf::MeshInterface::HEXA_8);
    REQUIRE(cell.index == 73);
    inf::Cell found_cell(*builder.mesh, cell.type, cell.index);
    REQUIRE(found_cell.contains(p));

    inf::experimental::MikeCavity cavity(builder.dimension);
    cavity.addCell(*builder.mesh, cell);

    auto inserted_node_id = builder.addNode(p);
    builder.replaceCavity(cavity, inserted_node_id);
    builder.sync();
    REQUIRE(builder.mesh->nodeCount() == original_node_count + 1);
}

TEST_CASE("Can create Mike's Cavity class with just a bunch of faces") {
    int dimension = 3;
    inf::experimental::MikeCavity cavity(dimension);

    // add two prisms that make a hex
    std::vector<std::vector<int>> prism_0 = {{0, 3, 1}, {4, 5, 7}, {0, 1, 5, 4}, {1, 3, 7, 5}, {4, 7, 3, 0}};
    std::vector<std::vector<int>> prism_1 = {{1, 3, 2}, {5, 6, 7}, {1, 2, 6, 5}, {2, 3, 7, 6}, {1, 5, 7, 3}};
    cavity.addFaces(prism_0);
    REQUIRE(cavity.exposed_faces.size() == 5);
    cavity.addFaces(prism_1);
    REQUIRE(cavity.exposed_faces.size() == 8);

    // require faces are reversed to point into the cavity
    REQUIRE(cavity.exposed_faces[0] == std::vector<int>{1, 3, 0});
}

TEST_CASE("Can create Mike's Cavity class from a mesh and a cell entry") {
    auto cart_mesh = inf::CartMesh::create(8, 8, 8);

    int dimension = 3;
    inf::experimental::MikeCavity cavity(dimension);
    cavity.addCell(*cart_mesh, {inf::MeshInterface::HEXA_8, 73});
    REQUIRE(cavity.exposed_faces.size() == 6);
    REQUIRE(cavity.cells.size() == 1);
    REQUIRE(cavity.cells.begin()->type == inf::MeshInterface::HEXA_8);
    REQUIRE(cavity.cells.begin()->index == 73);
}

TEST_CASE("Builder will set mode based on initial mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create2D(2, 2);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    REQUIRE(builder.dimension == 2);
}

TEST_CASE("refine a mesh to meet a metric", "[edge-splitting]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    auto& builder = thing.builder;
    auto& db = thing.db;

    auto calc_edge_length = buildAnisotropicCallback(0.1);
    auto mechanics = Mechanics(mp,"adapt");

    int max_passes = 1;
    for (int pass = 0; pass < max_passes; pass++) {
        mechanics.collapseAllEdges(builder, calc_edge_length, 0.5 * sqrt(2.0));
        builder.sync();
        mechanics.copyBuilderAndSyncAndVisualize( builder, calc_edge_length);

        mechanics.splitAllEdges(builder, calc_edge_length, 2.0);
        builder.sync();
        mechanics.copyBuilderAndSyncAndVisualize( builder, calc_edge_length);

        mechanics.swapAllEdges(builder, calc_edge_length);
        mechanics.copyBuilderAndSyncAndVisualize( builder, calc_edge_length);
        builder.sync();

        for (int n = 0; n < builder.mesh->nodeCount(); n++) {
            builder.smooth(n, calc_edge_length, db.get());
        }
        mechanics.copyBuilderAndSyncAndVisualize( builder, calc_edge_length);
        builder.sync();
    }
}

TEST_CASE("Pliant smoothing can put node back in center") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    auto& builder = thing.builder;
    auto mechanics = Mechanics(mp,"pliant");

    auto calc_edge_length = buildIsotropicCallback(2.0*sqrt(0.25));
    int new_node_id = mechanics.subdivideCell(builder, {inf::MeshInterface::QUAD_4, 0}, {0.51, 0.51, 0.0});
    builder.smooth(new_node_id, calc_edge_length, nullptr, 100);
    Parfait::Point<double> p = builder.mesh->node(new_node_id);
    REQUIRE(p[0] == Approx(0.50));
    REQUIRE(p[1] == Approx(0.50));
    REQUIRE(p[2] == Approx(0.0));
}

TEST_CASE("Pliant smoothing can repel from zero edge length", "[pliant]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    auto& builder = thing.builder;
    auto mechanics = Mechanics(mp,"pliant");

    auto calc_edge_length = buildIsotropicCallback(2.0*sqrt(0.25));
    int new_node_id = mechanics.subdivideCell(builder, {inf::MeshInterface::QUAD_4, 0}, {0.0, 0.0, 0.0});
    mechanics.copyBuilderAndSyncAndVisualize(builder, calc_edge_length);
    builder.smooth(new_node_id, calc_edge_length, nullptr, 100);
    mechanics.copyBuilderAndSyncAndVisualize(builder, calc_edge_length);
    Parfait::Point<double> p = builder.mesh->node(new_node_id);
    CHECK(p[0] == Approx(0.50));
    CHECK(p[1] == Approx(0.50));
    REQUIRE(p[2] == Approx(0.0));
}

TEST_CASE("Cavity will self destroy bounding entites ") {
    int dimension = 2;
    inf::experimental::MikeCavity cavity(dimension);
    auto cart_mesh = inf::CartMesh::create2DWithBarsAndNodes(1, 1);

    cavity.addCell(*cart_mesh, {inf::MeshInterface::QUAD_4, 0});
    cavity.addCell(*cart_mesh, {inf::MeshInterface::BAR_2, 0});
    cavity.cleanup();

    REQUIRE(cavity.exposed_faces.size() == 5);
}

TEST_CASE("Find neighbor of a face") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create2DWithBarsAndNodes(10, 10, {{0, 0}, {10, 10}});
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    inf::Cell test_cell(*builder.mesh, inf::MeshInterface::QUAD_4, 34);
    auto face_nodes = test_cell.nodesInBoundingEntity(0);

    auto neighbors = builder.findNeighborsOfFace(face_nodes);
    REQUIRE(neighbors.size() == 2);
    for (auto n : neighbors) {
        REQUIRE(not n.isNull());
    }
}

TEST_CASE("Can grow a cavity based on metric edge lengths", "[grow-cavity]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    int dimension = 2;
    auto cart_mesh = inf::CartMesh::create2DWithBarsAndNodes(10, 10, {{0, 0}, {10, 10}});
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    auto metric_at_point = [](Parfait::Point<double> p) {
        return Parfait::MetricTensor::metricFromEllipse({4, 0, 0}, {0, 2, 0}, {0, 0, 1});
    };

    auto calc_edge_length = [&](Parfait::Point<double> a, Parfait::Point<double> b) {
        auto metric_a = metric_at_point(a);
        auto metric_b = metric_at_point(b);
        auto m = Parfait::MetricTensor::logEuclideanAverage({metric_a, metric_b}, {0.5, 0.5});
        return Parfait::MetricTensor::edgeLength(m, (a - b));
    };

    auto host_cell = builder.findHostCellIdReallySlow({5, 5, 0.0});
    inf::experimental::MikeCavity cavity(dimension);
    cavity.addCell(*builder.mesh, host_cell);
    int new_node_id = builder.addNode({5, 5, 0.0});
    builder.growCavityAroundPoint_interior(new_node_id, calc_edge_length, cavity);
    cavity.cleanup();
    inf::shortcut::visualize("grow-cavity-before", mp, builder.mesh);
    cavity.cleanup();
    builder.replaceCavity(cavity, new_node_id);
    builder.sync();
    inf::shortcut::visualize("grow-cavity-replace", mp, builder.mesh);
}

TEST_CASE("Make a mesh with geometry association") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    REQUIRE_THROWS(thing.builder.deleteNode(0));

    Parfait::Point<double> p{0.5, 0, 0};
    int id_of_node_we_plan_to_delete = thing.builder.addNode(p);
    int other_new_node_id = thing.builder.addNode({0, .5, 0});
    inf::geometry::GeomAssociation g;
    g.type = inf::geometry::GeomAssociation::EDGE;
    g.index = 0;
    thing.builder.node_geom_association[id_of_node_we_plan_to_delete].push_back(g);
    g.index = 1;
    thing.builder.node_geom_association[other_new_node_id].push_back(g);
    thing.builder.deleteNode(id_of_node_we_plan_to_delete);
    thing.builder.sync();
    REQUIRE(thing.builder.node_geom_association[id_of_node_we_plan_to_delete].size() == 1);
    REQUIRE(thing.builder.node_geom_association[id_of_node_we_plan_to_delete].front().type ==
            inf::geometry::GeomAssociation::EDGE);
    REQUIRE(thing.builder.node_geom_association[id_of_node_we_plan_to_delete].front().index == 1);
}

TEST_CASE("On a quad with geometry, insert a node in a bad place and then smooth to put it in the middle",
          "[split-on-cad]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto thing = getInitialUnitQuad(mp);
    REQUIRE_THROWS(thing.builder.deleteNode(0));

    auto& builder = thing.builder;
    auto& db = thing.db;
    auto mechanics = Mechanics(mp,"split-on-cad");
    auto cells_containing_edge = mechanics.getCellsContainingEdge(*builder.mesh, {0, 1}, builder.dimension);
    inf::experimental::MikeCavity cavity(builder.dimension);
    for (auto c : cells_containing_edge) {
        cavity.addCell(*builder.mesh, c);
    }

    inf::geometry::GeomAssociation g;
    g.type = inf::geometry::GeomAssociation::EDGE;
    g.index = 0;
    auto p = db->getEdge(g.index)->project({0.25, 0, 0});
    printf("new node is at %s\n", p.to_string().c_str());
    int new_node_id = builder.addNode(p);
    builder.node_geom_association[new_node_id].push_back(g);
    builder.replaceCavity(cavity, new_node_id);
    auto calc_edge_length = [](Parfait::Point<double> a, Parfait::Point<double> b) { return a.distance(b) / 0.5; };
    builder.sync();
    inf::shortcut::visualize("split-edge-on-cad.0", mp, builder.mesh);
    REQUIRE(builder.mesh->node(new_node_id)[0] == Approx(0.25));
    builder.smooth(new_node_id, calc_edge_length, db.get());
    REQUIRE(builder.mesh->node(new_node_id)[0] == Approx(0.5).margin(0.02));

    inf::shortcut::visualize("split-edge-on-cad.1", mp, builder.mesh);
}

TEST_CASE("Builder can replace a cavity in a 2D mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create2DWithBarsAndNodes(1, 1);

    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));
    auto p = Parfait::Point<double>{0.5, 0.5, 0.0};
    int new_node_id = builder.addNode(p);
    inf::experimental::MikeCavity cavity(builder.dimension);
    cavity.addCell(*builder.mesh, {inf::MeshInterface::QUAD_4, 0});
    builder.replaceCavity(cavity, new_node_id);
    builder.sync();

    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::TRI_3) == 4);
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::QUAD_4) == 0);
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::BAR_2) == 4);
    for (int i = 0; i < 4; i++) {
        inf::Cell cell(*builder.mesh, inf::MeshInterface::TRI_3, i);
        auto points = cell.points();
        auto u = points[1] - points[0];
        auto v = points[2] - points[0];
        u.normalize();
        v.normalize();
        auto maybe_z = u.cross(v);
        maybe_z.normalize();
        REQUIRE(maybe_z[2] == Approx(1.0));
    }
}

TEST_CASE("Can create a mesh builder for a 2D mesh", "[builder-2d]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto cart_mesh = inf::CartMesh::create2D(2, 2);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    auto circle_points = Parfait::generateRandomPointsOnCircleInXYPlane(20, {0.5, 0.5, 0.0}, 0.2);
    inf::shortcut::visualize("cavity-2d-before", mp, builder.mesh);
    Parfait::PointWriter::write("circle-points", circle_points);
    int i = 0;
    for (auto p : circle_points) {
        builder.insertPoint(p);
        builder.sync();
        inf::shortcut::visualize("cavity-2d." + std::to_string(i++), mp, builder.mesh);
    }
    //    auto p = Parfait::Point<double>{0.25, 0.25, 0.0};
    //    builder.insertPoint(p);
}

TEST_CASE("edge flip cavity") {
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh_and_geometry = getInitialUnitQuad(mp);
    auto& builder = mesh_and_geometry.builder;

    inf::experimental::MikeCavity cavity(builder.dimension);
    cavity.addCell(*builder.mesh, {inf::MeshInterface::QUAD_4, 0});
    builder.replaceCavity(cavity, 0);
    builder.sync();
    // inf::shortcut::visualize("edge_flip_0.vtk",mp,builder.mesh);

    REQUIRE(2 == builder.mesh->cellCount(inf::MeshInterface::TRI_3));

    inf::experimental::MikeCavity cavity2(builder.dimension);
    cavity2.addCell(*builder.mesh, {inf::MeshInterface::TRI_3, 0});
    cavity2.addCell(*builder.mesh, {inf::MeshInterface::TRI_3, 1});
    builder.replaceCavity(cavity2, 1);
    builder.sync();
    REQUIRE(2 == builder.mesh->cellCount(inf::MeshInterface::TRI_3));
    // inf::shortcut::visualize("edge_flip_1.vtk",mp,builder.mesh);
}

TEST_CASE("Wind a set of faces") {
    std::vector<std::vector<int>> unwound_faces = { {1,2}, {0,1}, {2, 0}};
    auto wound_faces = inf::experimental::MeshBuilder::windFacesHeadToTail(unwound_faces);
    REQUIRE(wound_faces.size() == unwound_faces.size());

    int num_faces = wound_faces.size();
    for(int i = 0; i < num_faces; i++){
        auto& f = wound_faces[i] ;
        auto& next_f = wound_faces[(i+1) % num_faces];
        int tail_node = f.back();
        int next_head_node = next_f.front();

        REQUIRE(tail_node == next_head_node);
    }
}

