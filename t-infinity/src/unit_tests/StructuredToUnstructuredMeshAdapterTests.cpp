#include <RingAssertions.h>
#include <t-infinity/StructuredToUnstructuredMeshAdapter.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/StructuredBlockConnectivity.h>
#include <t-infinity/StructuredTinfMesh.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/AddHalo.h>
#include <t-infinity/Cell.h>

using namespace inf;

TEST_CASE("SMesh copy/assignent") {
    std::array<int, 3> dimensions = {2, 3, 4};
    Parfait::Point<double> lo = {0, 0, 0};
    Parfait::Point<double> hi = {1, 1, 1};
    int block_id = 3;
    SMesh mesh;
    mesh.add(std::make_shared<CartesianBlock>(lo, hi, dimensions, block_id));
    SECTION("Copy constructor") {
        auto new_mesh = mesh;
        REQUIRE(1 == new_mesh.blockIds().size());
        auto expected_dimensions = mesh.getBlock(block_id).nodeDimensions();
        auto actual_dimensions = mesh.getBlock(block_id).nodeDimensions();
        REQUIRE(expected_dimensions == actual_dimensions);
    }
    SECTION("Assignment") {
        SMesh new_mesh;
        new_mesh = mesh;
        REQUIRE(1 == new_mesh.blockIds().size());
        auto expected_dimensions = mesh.getBlock(block_id).nodeDimensions();
        auto actual_dimensions = mesh.getBlock(block_id).nodeDimensions();
        REQUIRE(expected_dimensions == actual_dimensions);
    }
}

TEST_CASE("Can convert a StructuredMesh into a TinfMesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    INFO("Rank: " << mp.Rank());
    auto mesh = std::make_shared<SMesh>();
    for (int block_id = 0; block_id < 4; ++block_id) {
        if (block_id % mp.NumberOfProcesses() == mp.Rank()) {
            printf("adding block %d to rank: %d\n", block_id, mp.Rank());
            std::array<int, 3> dimensions = {2, 3, 4};
            Parfait::Point<double> lo = {0 + double(block_id), 0, 0};
            Parfait::Point<double> hi = {1 + double(block_id), 1, 1};
            auto block = std::make_shared<CartesianBlock>(lo, hi, dimensions, block_id);
            mesh->add(block);
        }
    }
    StructuredBlockDimensions node_dimensions;
    StructuredBlockDimensions cell_dimensions;
    for (int block_id : mesh->blockIds()) {
        node_dimensions[block_id] = mesh->getBlock(block_id).nodeDimensions();
        for (int axis = 0; axis < 3; ++axis) cell_dimensions[block_id][axis] = node_dimensions[block_id][axis] - 1;
    }

    auto mesh_connectivity = buildMeshBlockConnectivity(mp, *mesh);
    auto global_nodes = assignStructuredMeshGlobalNodeIds(mp, mesh_connectivity, node_dimensions);
    auto global_cells = assignStructuredMeshGlobalCellIds(mp, cell_dimensions);

    auto tinf_mesh_ptr =
        convertStructuredMeshToUnstructuredMesh(mp.getCommunicator(), *mesh, global_nodes, global_cells);
    const auto& tinf_mesh = *tinf_mesh_ptr;
    REQUIRE(mp.Rank() == tinf_mesh.partitionId());

    auto getOwnedCellCount = [&](MeshInterface::CellType type) {
        int count = 0;
        for (int cell_id = 0; cell_id < tinf_mesh.cellCount(); ++cell_id) {
            if (tinf_mesh.cellOwner(cell_id) != tinf_mesh.partitionId()) continue;
            if (tinf_mesh.cellType(cell_id) != type) continue;
            count++;
        }
        return count;
    };

    auto g2l_cell = GlobalToLocal::buildCell(tinf_mesh);
    auto g2l_node = GlobalToLocal::buildNode(tinf_mesh);

    int cell_count = 0;
    for (auto block_id : mesh->blockIds()) {
        const auto& block = mesh->getBlock(block_id);
        auto d = block.nodeDimensions();
        loopMDRange({0, 0, 0}, d, [&](int i, int j, int k) {
            auto global_node_id = global_nodes.ids.at(block_id)(i, j, k);
            if (mp.Rank() != global_nodes.owner.getOwningRank(global_node_id) or
                block_id == global_nodes.owner.getOwningBlock(global_node_id)) {
                auto expected_point = block.point(i, j, k);
                auto actual_point = Parfait::Point<double>(tinf_mesh.node(g2l_node.at(global_node_id)));
                INFO("expected: " << expected_point.to_string() << " actual: " << actual_point.to_string());
                REQUIRE(expected_point.approxEqual(actual_point));
            }
        });

        std::array<int, 8> actual_cell_local_node_ids{};
        std::array<long, 8> expected_cell_global_node_ids{};
        for (auto& dim : d) dim -= 1;
        const auto& global_node_ids = global_nodes.ids.at(block_id);
        const auto& global_cell_ids = global_cells.ids.at(block_id);
        auto cell_g2l = GlobalToLocal::buildCell(tinf_mesh);
        loopMDRange({0, 0, 0}, d, [&](int i, int j, int k) {
            int cell_id = cell_g2l.at(global_cell_ids(i, j, k));
            if (tinf_mesh.cellOwner(cell_id) != tinf_mesh.partitionId()) return;
            if (MeshInterface::is2DCellType(tinf_mesh.cellType(cell_id))) return;
            INFO("I: " << i << " J: " << j << " K: " << k);
            expected_cell_global_node_ids[0] = global_node_ids(i, j, k);
            expected_cell_global_node_ids[1] = global_node_ids(i + 1, j, k);
            expected_cell_global_node_ids[2] = global_node_ids(i + 1, j + 1, k);
            expected_cell_global_node_ids[3] = global_node_ids(i, j + 1, k);
            expected_cell_global_node_ids[4] = global_node_ids(i, j, k + 1);
            expected_cell_global_node_ids[5] = global_node_ids(i + 1, j, k + 1);
            expected_cell_global_node_ids[6] = global_node_ids(i + 1, j + 1, k + 1);
            expected_cell_global_node_ids[7] = global_node_ids(i, j + 1, k + 1);
            tinf_mesh.cell(cell_id, actual_cell_local_node_ids.data());
            for (int corner = 0; corner < 8; ++corner) {
                long expected_gid = expected_cell_global_node_ids[corner];
                long actual_gid = tinf_mesh.globalNodeId(actual_cell_local_node_ids[corner]);
                REQUIRE(expected_gid == actual_gid);
            }
            cell_count += 1;
        });
    }

    int expected_owned_hex_count = 0;
    for (const auto& p : cell_dimensions) {
        const auto& d = p.second;
        expected_owned_hex_count += d[0] * d[1] * d[2];
    }

    REQUIRE(expected_owned_hex_count == cell_count);

    int expected_surface_cell_count = 0;
    for (int block_id : mesh->blockIds()) {
        for (auto side : inf::AllBlockSides) {
            if (mesh_connectivity.at(block_id).count(side) == 0) {
                auto dimensions = global_cells.ids.at(block_id).dimensions();
                dimensions[sideToAxis(side)] = 1;
                expected_surface_cell_count += dimensions[0] * dimensions[1] * dimensions[2];
            }
        }
    }
    REQUIRE(expected_surface_cell_count == getOwnedCellCount(MeshInterface::QUAD_4));

    for (int cell_id = 0; cell_id < tinf_mesh.cellCount(); ++cell_id) {
        if (tinf_mesh.cellType(cell_id) != MeshInterface::QUAD_4) continue;
        auto cell = Cell(tinf_mesh, cell_id);
        auto side = static_cast<BlockSide>(tinf_mesh.cellTag(cell_id) % 10) - 1;
        auto n = cell.faceAreaNormal(0);
        n.normalize();
        INFO("n: " + n.to_string() << " side: " << side);
        switch (side) {
            case inf::IMIN:
                REQUIRE(n.approxEqual({-1, 0, 0}));
                break;
            case inf::IMAX:
                REQUIRE(n.approxEqual({1, 0, 0}));
                break;
            case inf::JMIN:
                REQUIRE(n.approxEqual({0, -1, 0}));
                break;
            case inf::JMAX:
                REQUIRE(n.approxEqual({0, 1, 0}));
                break;
            case inf::KMIN:
                REQUIRE(n.approxEqual({0, 0, -1}));
                break;
            default:
                REQUIRE(n.approxEqual({0, 0, 1}));
                break;
        }
    }

    int expected_node_count = mp.NumberOfProcesses() == 2 ? 160 : tinf_mesh.nodeCount();
    int expected_owned_cell_count = expected_owned_hex_count + getOwnedCellCount(MeshInterface::QUAD_4);

    REQUIRE(expected_node_count == tinf_mesh.nodeCount());
    REQUIRE(expected_owned_cell_count == cell_count + expected_surface_cell_count);
}

TEST_CASE("Can build a FieldInterface from a StructuredField") {
    auto structured_field = SField("structured-data", FieldAttributes::Node());
    auto block1 = structured_field.createBlock(std::array<int, 3>{5, 5, 5}, 0);
    auto block2 = structured_field.createBlock(std::array<int, 3>{2, 2, 2}, 1);
    structured_field.add(block1);
    structured_field.add(block2);
    auto unstructured_field = buildFieldInterfaceFromStructuredField(structured_field);
    int expected_size = 5 * 5 * 5 + 2 * 2 * 2;
    REQUIRE(expected_size == unstructured_field->size());
}

std::shared_ptr<StructuredMesh> buildStrMesh(MessagePasser mp) {
    auto mesh = std::make_shared<SMesh>();
    int block_id = 0;
    loopMDRange({0, 0, 0}, std::array<int, 3>{2, 2, 2}, [&](int x, int y, int z) {
        if (block_id % mp.NumberOfProcesses() == mp.Rank()) {
            std::array<int, 3> dimensions = {2, 2, 2};
            Parfait::Point<double> lo = {double(x), double(y), double(z)};
            Parfait::Point<double> hi = {lo[0] + 1, lo[1] + 1, lo[2] + 1};
            auto block = std::make_shared<CartesianBlock>(lo, hi, dimensions, block_id);
            mesh->add(block);
        }
        block_id++;
    });
    return mesh;
}

TEST_CASE("Can create a unstructured mesh from a structured mesh with two levels of ghost cells") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = buildStrMesh(mp);
    auto connectivity = buildMeshBlockConnectivity(mp, *mesh);
    auto unstructured_mesh = convertStructuredMeshToUnstructuredMesh(mp, *mesh, connectivity);
    auto final_mesh = addHaloCells(mp, *unstructured_mesh);
    REQUIRE(64 == final_mesh->cellCount(MeshInterface::HEXA_8));
    REQUIRE(unstructured_mesh->countOwnedCells() == final_mesh->countOwnedCells());
}

TEST_CASE("Get opposite cell face") {
    auto mesh = CartMesh::createVolume(1, 1, 1);
    Cell hex(*mesh, 0);
    REQUIRE(MeshInterface::HEXA_8 == hex.type());
    for (auto side : AllBlockSides) {
        auto opposite_side = getOppositeSide(side);
        auto f1 = BlockSideToHexOrdering[side];
        auto f2 = BlockSideToHexOrdering[opposite_side];
        auto n1 = hex.faceAreaNormal(f1);
        auto n2 = hex.faceAreaNormal(f2);
        REQUIRE(n1.approxEqual(-1.0 * n2));
    }
}

TEST_CASE("Hex edge offset") {
    std::array<std::array<BlockSide, 2>, 12> edge_to_sides = {{{JMIN, KMIN},
                                                               {IMAX, KMIN},
                                                               {JMAX, KMIN},
                                                               {IMIN, KMIN},
                                                               {IMIN, JMIN},
                                                               {IMAX, JMIN},
                                                               {IMAX, JMAX},
                                                               {IMIN, JMAX},
                                                               {JMIN, KMAX},
                                                               {IMAX, KMAX},
                                                               {JMAX, KMAX},
                                                               {IMIN, KMAX}}};
    for (int edge = 0; edge < 12; ++edge) {
        std::array<int, 3> edge_offset{};
        edge_offset = shiftIJK(edge_offset, edge_to_sides[edge].front(), 1);
        edge_offset = shiftIJK(edge_offset, edge_to_sides[edge].back(), 1);
        REQUIRE(edge_offset == HexEdgeNeighborOffset[edge]);
    }
}
TEST_CASE("Hex corner offset") {
    std::array<std::array<BlockSide, 3>, 12> corner_to_sides = {{{IMIN, JMIN, KMIN},
                                                                 {IMAX, JMIN, KMIN},
                                                                 {IMAX, JMAX, KMIN},
                                                                 {IMIN, JMAX, KMIN},
                                                                 {IMIN, JMIN, KMAX},
                                                                 {IMAX, JMIN, KMAX},
                                                                 {IMAX, JMAX, KMAX},
                                                                 {IMIN, JMAX, KMAX}}};
    for (int corner = 0; corner < 8; ++corner) {
        std::array<int, 3> corner_offset{};
        for (auto side : corner_to_sides[corner]) corner_offset = shiftIJK(corner_offset, side, 1);
        REQUIRE(corner_offset == HexCornerNeighborOffset[corner]);
    }
}

TEST_CASE("Can shift ijk") {
    std::array<int, 3> ijk = {1, 1, 1};
    REQUIRE(std::array<int, 3>{0, 1, 1} == shiftIJK(ijk, IMIN, 1));
    REQUIRE(std::array<int, 3>{2, 1, 1} == shiftIJK(ijk, IMAX, 1));
    REQUIRE(std::array<int, 3>{1, 0, 1} == shiftIJK(ijk, JMIN, 1));
    REQUIRE(std::array<int, 3>{1, 2, 1} == shiftIJK(ijk, JMAX, 1));
    REQUIRE(std::array<int, 3>{1, 1, 0} == shiftIJK(ijk, KMIN, 1));
    REQUIRE(std::array<int, 3>{1, 1, 2} == shiftIJK(ijk, KMAX, 1));
}

TEST_CASE("Assign global cell ids back to structured mesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    const auto mesh = buildStrMesh(mp);
    const auto connectivity = buildMeshBlockConnectivity(mp, *mesh);
    const auto global_nodes = assignUniqueGlobalNodeIds(mp, *mesh);
    const auto global_cells = assignStructuredMeshGlobalCellIds(mp, *mesh);
    const auto tinf_mesh =
        addHaloCells(mp, *convertStructuredMeshToUnstructuredMesh(mp, *mesh, global_nodes, global_cells));

    int num_ghost_cells_per_block_side = 2;
    auto global_cells_with_ghosts =
        getGlobalCellIdsWithGhostCells(mp, global_cells, *tinf_mesh, num_ghost_cells_per_block_side);

    for (int block_id : mesh->blockIds()) {
        const auto& without_ghost_cells = global_cells.ids.at(block_id).dimensions();
        const auto& with_ghost_cells = global_cells_with_ghosts.at(block_id).dimensions();
        for (int axis = 0; axis < 3; ++axis) {
            int expected = without_ghost_cells[axis] + 2 * num_ghost_cells_per_block_side;
            int actual = with_ghost_cells[axis];
            REQUIRE(expected == actual);
        }
    }

    for (int block_id : mesh->blockIds()) {
        const auto& without_ghost_cells = global_cells.ids.at(block_id);
        const auto& with_ghost_cells = global_cells_with_ghosts.at(block_id);

        // Do the owned global ids match?
        loopMDRange({0, 0, 0}, without_ghost_cells.dimensions(), [&](int i, int j, int k) {
            INFO("block: " << block_id << " (" << i << "," << j << "," << k << ")");
            long expected = without_ghost_cells(i, j, k);
            long actual = with_ghost_cells(i + num_ghost_cells_per_block_side,
                                           j + num_ghost_cells_per_block_side,
                                           k + num_ghost_cells_per_block_side);
            REQUIRE(expected == actual);
        });

        for (const auto& block_face : connectivity.at(block_id)) {
            auto side = block_face.first;
            int neighbor_block_id = block_face.second.neighbor_block_id;
            auto axis = sideToAxis(side);
            std::array<int, 3> start{};
            std::array<int, 3> end = without_ghost_cells.dimensions();
            for (int direction = 0; direction < 3; ++direction) {
                start[direction] += num_ghost_cells_per_block_side;
                end[direction] += num_ghost_cells_per_block_side;
            }
            start[axis] = isMaxFace(side) ? end[axis] : 0;
            end[axis] = start[axis] + num_ghost_cells_per_block_side;
            loopMDRange(start, end, [&](int i, int j, int k) {
                INFO("block: " << block_id << " side: " << side << " (" << i << "," << j << "," << k << ")");
                auto cell_gid = with_ghost_cells(i, j, k);
                REQUIRE(neighbor_block_id == global_cells.owner.getOwningBlock(cell_gid));
            });
        }
    }

    auto getGlobal = [&](int block_id, int i, int j, int k) {
        long gid;
        int owning_rank = global_cells.owner.getOwningRankOfBlock(block_id);
        if (mp.Rank() == owning_rank) gid = global_cells_with_ghosts.at(block_id)(i, j, k);
        mp.Broadcast(gid, owning_rank);
        return gid;
    };

    // Check that the edges match
    REQUIRE(getGlobal(0, 4, 2, 4) == getGlobal(5, 2, 2, 2));
    REQUIRE(getGlobal(0, 4, 3, 4) == getGlobal(5, 2, 3, 2));
    REQUIRE(getGlobal(0, 3, 2, 3) == getGlobal(5, 1, 2, 1));
    REQUIRE(getGlobal(0, 3, 3, 3) == getGlobal(5, 1, 3, 1));

    // Check that the corners match
    REQUIRE(getGlobal(0, 4, 4, 4) == getGlobal(7, 2, 2, 2));
    REQUIRE(getGlobal(0, 3, 3, 3) == getGlobal(7, 1, 1, 1));
}

TEST_CASE("Assign global node ids + ghosts") {
    MessagePasser mp(MPI_COMM_WORLD);
    const auto mesh = buildStrMesh(mp);
    const auto connectivity = buildMeshBlockConnectivity(mp, *mesh);
    const auto global_nodes = assignStructuredMeshGlobalNodeIds(mp, connectivity, *mesh);
    const auto global_cells = assignStructuredMeshGlobalCellIds(mp, *mesh);
    const auto tinf_mesh =
        addHaloCells(mp, *convertStructuredMeshToUnstructuredMesh(mp, *mesh, global_nodes, global_cells));

    int num_ghost_layers = 1;
    auto global_cells_with_ghosts = StructuredMeshGlobalIds{
        getGlobalCellIdsWithGhostCells(mp, global_cells, *tinf_mesh, num_ghost_layers), global_cells.owner};

    auto global_nodes_with_ghosts = getGlobalNodeIdsWithGhostNodes(mp, global_cells_with_ghosts, *tinf_mesh);
    REQUIRE(mesh->blockIds().size() == global_nodes_with_ghosts.size());
    for (int block_id : mesh->blockIds()) {
        INFO("Block ID: " << block_id);
        auto expected_dimensions = mesh->getBlock(block_id).nodeDimensions();
        for (int& d : expected_dimensions) d += 2 * num_ghost_layers;
        const auto& nodes = global_nodes_with_ghosts.at(block_id);
        REQUIRE(expected_dimensions == nodes.dimensions());
        const auto& original_gids = global_nodes.ids.at(block_id);
        loopMDRange({0, 0, 0}, original_gids.dimensions(), [&](int i, int j, int k) {
            INFO("Global node ID changed for (" << i << "," << j << "," << k << ")");
            REQUIRE(original_gids(i, j, k) == nodes(i + num_ghost_layers, j + num_ghost_layers, k + num_ghost_layers));
        });

        for (const auto& f : connectivity.at(block_id)) {
            int expected_block_id = f.second.neighbor_block_id;

            // Check only the nodes that should be owned by the neighbor
            std::array<int, 3> start{}, end{};
            for (int i = 0; i < 3; ++i) {
                start[i] = num_ghost_layers + 1;
                end[i] = expected_dimensions[i] - num_ghost_layers - 1;
            }
            auto axis = sideToAxis(f.first);
            start[axis] = isMaxFace(f.first) ? expected_dimensions[axis] - 1 : 0;
            end[axis] = start[axis] + 1;
            loopMDRange(start, end, [&](int i, int j, int k) {
                INFO("Node block owner incorrect for (" << i << "," << j << "," << k << ")");
                auto gid = global_nodes_with_ghosts.at(block_id)(i, j, k);
                auto actual_block_id = global_nodes.owner.getOwningBlock(gid);
                CHECK(expected_block_id == actual_block_id);
            });
        }
    }
}

TEST_CASE("Can encode/decode surface cell tags") {
    int block_id = 12;
    for (auto side : AllBlockSides) {
        auto tag = StructuredMeshSurfaceCellTag::encodeTag(block_id, side);
        REQUIRE((block_id + 1) * 10 + (side + 1) == tag);
        REQUIRE(block_id == StructuredMeshSurfaceCellTag::getBlockId(tag));
        REQUIRE(side == StructuredMeshSurfaceCellTag::getBlockSide(tag));
    }
}

TEST_CASE("Can build StructuredTinfMesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    const auto mesh = buildStrMesh(mp);
    auto connectivity = buildMeshBlockConnectivity(mp, *mesh);
    auto global_nodes = assignStructuredMeshGlobalNodeIds(mp, connectivity, *mesh);
    auto global_cells = assignStructuredMeshGlobalCellIds(mp, *mesh);
    auto tinf_mesh = convertStructuredMeshToUnstructuredMesh(mp, *mesh, global_nodes, global_cells);
    auto structured_tinf_mesh = StructuredTinfMesh(*tinf_mesh, global_nodes.ids, global_cells.ids);
    REQUIRE(mesh->blockIds() == structured_tinf_mesh.blockIds());
    SECTION("Unstructured -> Structured Mapping") {
        for (int block_id : mesh->blockIds()) {
            const auto& block = mesh->getBlock(block_id);
            loopMDRange({0, 0, 0}, block.nodeDimensions(), [&](int i, int j, int k) {
                auto expected = block.point(i, j, k);
                auto node_id = structured_tinf_mesh.nodeId(block_id, i, j, k);
                auto actual = structured_tinf_mesh.node(node_id);
                REQUIRE(expected.approxEqual(actual));
            });
        }
    }
    SECTION("Can create a StructuredBlock") {
        for (int block_id : mesh->blockIds()) {
            const auto& expected_block = mesh->getBlock(block_id);
            const auto& actual_block = structured_tinf_mesh.getBlock(block_id);
            REQUIRE(expected_block.nodeDimensions() == actual_block.nodeDimensions());
            REQUIRE(expected_block.cellDimensions() == actual_block.cellDimensions());
            loopMDRange({0, 0, 0}, expected_block.nodeDimensions(), [&](int i, int j, int k) {
                auto expected = expected_block.point(i, j, k);
                auto actual = actual_block.point(i, j, k);
                REQUIRE(expected.approxEqual(actual));
            });
        }
    }
    SECTION("Can shallow copy to a StructuredMesh") {
        auto shallow_copied_mesh = structured_tinf_mesh.shallowCopyToStructuredMesh();
        for (int block_id : mesh->blockIds()) {
            const auto& expected_block = mesh->getBlock(block_id);
            const auto& actual_block = shallow_copied_mesh->getBlock(block_id);
            REQUIRE(expected_block.nodeDimensions() == actual_block.nodeDimensions());
            REQUIRE(expected_block.cellDimensions() == actual_block.cellDimensions());
            loopMDRange({0, 0, 0}, expected_block.nodeDimensions(), [&](int i, int j, int k) {
                auto expected = expected_block.point(i, j, k);
                auto actual = actual_block.point(i, j, k);
                REQUIRE(expected.approxEqual(actual));
            });
        }
    }
    SECTION("Can create a StructuredBlockField") {
        std::vector<double> node_ids(structured_tinf_mesh.nodeCount());
        for (int node_id = 0; node_id < structured_tinf_mesh.nodeCount(); ++node_id) node_ids[node_id] = node_id;
        VectorFieldAdapter field("node ids", FieldAttributes::Node(), node_ids);
        for (int block_id : mesh->blockIds()) {
            auto block_field = structured_tinf_mesh.getField(block_id, field);
            auto expected_dimensions = mesh->getBlock(block_id).nodeDimensions();
            loopMDRange({0, 0, 0}, expected_dimensions, [&](int i, int j, int k) {
                auto expected = structured_tinf_mesh.nodeId(block_id, i, j, k);
                auto actual = int(block_field->value(i, j, k));
                REQUIRE(expected == actual);
            });
        }
    }
}