#include <parfait/CartBlock.h>
#include "CartMesh.h"
#include "FilterFactory.h"
#include "MeshShuffle.h"

void fillVolumeCells(const Parfait::CartBlock& block,
                     inf::TinfMeshData& data,
                     long& next_cell_gid,
                     long& next_node_gid,
                     int volume_tag,
                     int partition_id) {
    auto type = inf::MeshInterface::HEXA_8;
    auto& hexs = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];
    hexs.resize(block.numberOfCells() * 8);
    tags.resize(block.numberOfCells());
    owner.resize(block.numberOfCells());
    global_cell_ids.resize(block.numberOfCells());
    data.node_owner.resize(block.numberOfNodes(), partition_id);
    data.points.resize(block.numberOfNodes());
    data.global_node_id.resize(block.numberOfNodes());

    for (int c = 0; c < block.numberOfCells(); c++) {
        auto cell_nodes = block.getNodesInCell(c);
        for (int i = 0; i < 8; i++) hexs[8 * c + i] = cell_nodes[i];
        tags[c] = volume_tag;
        owner[c] = partition_id;
        global_cell_ids[c] = next_cell_gid++;
    }

    for (int n = 0; n < block.numberOfNodes(); n++) {
        block.getNode(n, data.points[n].data());
        data.global_node_id[n] = next_node_gid++;
    }
}

void fillQuads2D(const Parfait::CartBlock& block,
                 inf::TinfMeshData& data,
                 long& next_cell_gid,
                 int partition_id = 0) {
    auto type = inf::MeshInterface::QUAD_4;
    auto& quads = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];

    for (int i = 0; i < block.numberOfCells_X(); i++) {
        for (int j = 0; j < block.numberOfCells_Y(); j++) {
            quads.push_back(block.convert_ijk_ToNodeId(i, j, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j + 1, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i, j + 1, 0));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(0);
        }
    }
}

void fillEdges2D(const Parfait::CartBlock& block,
                 inf::TinfMeshData& data,
                 long& next_cell_gid,
                 int partition_id = 0) {
    auto type = inf::MeshInterface::BAR_2;
    auto& bars = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];

    int imax = block.numberOfCells_X();
    int jmax = block.numberOfCells_Y();
    for (int i = 0; i < block.numberOfCells_X(); i++) {
        bars.push_back(block.convert_ijk_ToNodeId(i, 0, 0));
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, 0, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(0);
    }

    for (int i = block.numberOfCells_X() - 1; i >= 0; i--) {
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, jmax, 0));
        bars.push_back(block.convert_ijk_ToNodeId(i, jmax, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(2);
    }

    for (int j = 0; j < block.numberOfCells_Y(); j++) {
        bars.push_back(block.convert_ijk_ToNodeId(imax, j, 0));
        bars.push_back(block.convert_ijk_ToNodeId(imax, j + 1, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(1);
    }

    for (int j = block.numberOfCells_Y() - 1; j >= 0; j--) {
        bars.push_back(block.convert_ijk_ToNodeId(0, j + 1, 0));
        bars.push_back(block.convert_ijk_ToNodeId(0, j, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(3);
    }
}

void fillNodeCells2D(const Parfait::CartBlock& block,
                     inf::TinfMeshData& data,
                     long& next_cell_gid,
                     int partition_id = 0) {
    auto type = inf::MeshInterface::NODE;
    auto& nodes = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];

    int imax = block.numberOfCells_X();
    int jmax = block.numberOfCells_Y();
    nodes.push_back(block.convert_ijk_ToNodeId(0, 0, 0));
    nodes.push_back(block.convert_ijk_ToNodeId(imax, 0, 0));
    nodes.push_back(block.convert_ijk_ToNodeId(imax, jmax, 0));
    nodes.push_back(block.convert_ijk_ToNodeId(0, jmax, 0));
    for (int i = 0; i < 4; i++) {
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(0);
    }
}

void fillNodes2D(const Parfait::CartBlock& block, inf::TinfMeshData& mesh) {
    long next_node_gid = 0;
    int num_nodes = block.numberOfNodes() / 2;
    mesh.points.resize(num_nodes);
    mesh.global_node_id.resize(num_nodes);
    int rank_zero = 0;
    mesh.node_owner.resize(num_nodes, rank_zero);
    for (int n = 0; n < num_nodes; n++) {
        auto p = block.getPoint(n);
        mesh.points[n][0] = p[0];
        mesh.points[n][1] = p[1];
        mesh.points[n][2] = 0.0;
        mesh.global_node_id[n] = next_node_gid++;
    }
}

std::shared_ptr<inf::TinfMesh> inf::CartMesh::create2D(int num_x,
                                                       int num_y,
                                                       Parfait::Extent<double, 2> e) {
    Parfait::CartBlock block({{e.lo[0], e.lo[1], 0}, {e.hi[0], e.hi[1], 1}}, num_x, num_y, 1);
    TinfMeshData data;
    long next_cell_gid = 0;
    int partition_id = 0;

    fillNodes2D(block, data);
    fillQuads2D(block, data, next_cell_gid, partition_id);
    fillEdges2D(block, data, next_cell_gid, partition_id);

    auto mesh = std::make_shared<TinfMesh>(data, partition_id);
    return mesh;
}

std::shared_ptr<inf::TinfMesh> inf::CartMesh::create2DWithBars(int num_x,
                                                               int num_y,
                                                               Parfait::Extent<double, 2> e) {
    Parfait::CartBlock block({{e.lo[0], e.lo[1], 0}, {e.hi[0], e.hi[1], 1}}, num_x, num_y, 1);
    TinfMeshData data;
    long next_cell_gid = 0;
    int partition_id = 0;

    fillNodes2D(block, data);
    fillQuads2D(block, data, next_cell_gid, partition_id);
    fillEdges2D(block, data, next_cell_gid, partition_id);

    auto mesh = std::make_shared<TinfMesh>(data, partition_id);
    return mesh;
}

std::shared_ptr<inf::TinfMesh> inf::CartMesh::create2DWithBarsAndNodes(
    int num_x, int num_y, Parfait::Extent<double, 2> e) {
    Parfait::CartBlock block({{e.lo[0], e.lo[1], 0}, {e.hi[0], e.hi[1], 1}}, num_x, num_y, 1);
    TinfMeshData data;
    long next_cell_gid = 0;
    int partition_id = 0;

    fillNodes2D(block, data);
    fillQuads2D(block, data, next_cell_gid, partition_id);
    fillEdges2D(block, data, next_cell_gid, partition_id);
    fillNodeCells2D(block, data, next_cell_gid, partition_id);

    auto mesh = std::make_shared<TinfMesh>(data, partition_id);
    return mesh;
}

void fillSurfaceCells(const Parfait::CartBlock& block,
                      inf::TinfMeshData& data,
                      long& next_cell_gid,
                      int partition_id = 0) {
    auto type = inf::MeshInterface::QUAD_4;
    auto& quads = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];

    // bottom
    int bottom_tag = 1;
    for (int i = 0; i < block.numberOfCells_X(); i++) {
        for (int j = 0; j < block.numberOfCells_Y(); j++) {
            quads.push_back(block.convert_ijk_ToNodeId(i, j, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i, j + 1, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j + 1, 0));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j, 0));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(bottom_tag);
        }
    }

    // top
    int top_tag = 6;
    int num_z = block.numberOfNodes_Z() - 1;
    for (int i = 0; i < block.numberOfCells_X(); i++) {
        for (int j = 0; j < block.numberOfCells_Y(); j++) {
            quads.push_back(block.convert_ijk_ToNodeId(i, j, num_z));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j, num_z));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, j + 1, num_z));
            quads.push_back(block.convert_ijk_ToNodeId(i, j + 1, num_z));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(top_tag);
        }
    }

    // front
    int front_tag = 2;
    for (int j = 0; j < block.numberOfCells_Y(); j++) {
        for (int k = 0; k < block.numberOfCells_Z(); k++) {
            quads.push_back(block.convert_ijk_ToNodeId(0, j, k));
            quads.push_back(block.convert_ijk_ToNodeId(0, j, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(0, j + 1, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(0, j + 1, k));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(front_tag);
        }
    }

    // back
    int back_tag = 4;
    int num_x = block.numberOfNodes_X() - 1;
    for (int j = 0; j < block.numberOfCells_Y(); j++) {
        for (int k = 0; k < block.numberOfCells_Z(); k++) {
            quads.push_back(block.convert_ijk_ToNodeId(num_x, j, k));
            quads.push_back(block.convert_ijk_ToNodeId(num_x, j + 1, k));
            quads.push_back(block.convert_ijk_ToNodeId(num_x, j + 1, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(num_x, j, k + 1));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(back_tag);
        }
    }

    // right
    int right_tag = 3;
    for (int i = 0; i < block.numberOfCells_X(); i++) {
        for (int k = 0; k < block.numberOfCells_Z(); k++) {
            quads.push_back(block.convert_ijk_ToNodeId(i, 0, k));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, 0, k));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, 0, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(i, 0, k + 1));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(right_tag);
        }
    }

    // left
    int left_tag = 5;
    int num_y = block.numberOfNodes_Y() - 1;
    for (int i = 0; i < block.numberOfCells_X(); i++) {
        for (int k = 0; k < block.numberOfCells_Z(); k++) {
            quads.push_back(block.convert_ijk_ToNodeId(i, num_y, k));
            quads.push_back(block.convert_ijk_ToNodeId(i, num_y, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, num_y, k + 1));
            quads.push_back(block.convert_ijk_ToNodeId(i + 1, num_y, k));
            owner.push_back(partition_id);
            global_cell_ids.push_back(next_cell_gid++);
            tags.push_back(left_tag);
        }
    }
}
void fillNodeCells(const Parfait::CartBlock& block,
                   inf::TinfMeshData& data,
                   long& next_cell_gid,
                   int partition_id = 0) {
    auto type = inf::MeshInterface::NODE;
    auto& nodes = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];
    int imax = block.numberOfCells_X();
    int jmax = block.numberOfCells_Y();
    int kmax = block.numberOfCells_Z();

    nodes.push_back(block.convert_ijk_ToNodeId(0, 0, 0));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(1);

    nodes.push_back(block.convert_ijk_ToNodeId(imax, 0, 0));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(2);

    nodes.push_back(block.convert_ijk_ToNodeId(imax, jmax, 0));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(3);

    nodes.push_back(block.convert_ijk_ToNodeId(0, jmax, 0));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(4);

    nodes.push_back(block.convert_ijk_ToNodeId(0, 0, kmax));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(5);

    nodes.push_back(block.convert_ijk_ToNodeId(imax, 0, kmax));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(6);

    nodes.push_back(block.convert_ijk_ToNodeId(imax, jmax, kmax));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(7);

    nodes.push_back(block.convert_ijk_ToNodeId(0, jmax, kmax));
    owner.push_back(partition_id);
    global_cell_ids.push_back(next_cell_gid++);
    tags.push_back(8);
}

void fillEdgeCells(const Parfait::CartBlock& block,
                   inf::TinfMeshData& data,
                   long& next_cell_gid,
                   int partition_id = 0) {
    auto type = inf::MeshInterface::BAR_2;
    auto& bars = data.cells[type];
    auto& tags = data.cell_tags[type];
    auto& owner = data.cell_owner[type];
    auto& global_cell_ids = data.global_cell_id[type];

    int imax = block.numberOfCells_X();
    int jmax = block.numberOfCells_Y();
    int kmax = block.numberOfCells_Z();
    for (int i = 0; i < imax; i++) {
        // edge 0
        bars.push_back(block.convert_ijk_ToNodeId(i, 0, 0));
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, 0, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(1);

        // edge 2
        bars.push_back(block.convert_ijk_ToNodeId(i, jmax, 0));
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, jmax, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(2);

        // edge 8
        bars.push_back(block.convert_ijk_ToNodeId(i, 0, kmax));
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, 0, kmax));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(10);

        // edge 10
        bars.push_back(block.convert_ijk_ToNodeId(i, jmax, kmax));
        bars.push_back(block.convert_ijk_ToNodeId(i + 1, jmax, kmax));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(10);
    }

    for (int j = 0; j < jmax; j++) {
        // edge 1
        bars.push_back(block.convert_ijk_ToNodeId(imax, j, 0));
        bars.push_back(block.convert_ijk_ToNodeId(imax, j + 1, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(1);

        // edge 3
        bars.push_back(block.convert_ijk_ToNodeId(0, j, 0));
        bars.push_back(block.convert_ijk_ToNodeId(0, j + 1, 0));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(3);

        // edge 9
        bars.push_back(block.convert_ijk_ToNodeId(0, j, kmax));
        bars.push_back(block.convert_ijk_ToNodeId(0, j + 1, kmax));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(9);

        // edge 11
        bars.push_back(block.convert_ijk_ToNodeId(imax, j, kmax));
        bars.push_back(block.convert_ijk_ToNodeId(imax, j + 1, kmax));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(11);
    }

    for (int k = 0; k < kmax; k++) {
        // edge 4
        bars.push_back(block.convert_ijk_ToNodeId(0, 0, k));
        bars.push_back(block.convert_ijk_ToNodeId(0, 0, k + 1));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(4);

        // edge 5
        bars.push_back(block.convert_ijk_ToNodeId(imax, 0, k));
        bars.push_back(block.convert_ijk_ToNodeId(imax, 0, k + 1));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(5);

        // edge 6
        bars.push_back(block.convert_ijk_ToNodeId(imax, jmax, k));
        bars.push_back(block.convert_ijk_ToNodeId(imax, jmax, k + 1));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(6);

        // edge 7
        bars.push_back(block.convert_ijk_ToNodeId(0, jmax, k));
        bars.push_back(block.convert_ijk_ToNodeId(0, jmax, k + 1));
        owner.push_back(partition_id);
        global_cell_ids.push_back(next_cell_gid++);
        tags.push_back(7);
    }
}

std::shared_ptr<inf::TinfMesh> inf::CartMesh::createVolume(int num_cells_x,
                                                           int num_cells_y,
                                                           int num_cells_z,
                                                           Parfait::Extent<double> e) {
    Parfait::CartBlock block(e, num_cells_x, num_cells_y, num_cells_z);
    TinfMeshData data;
    long cell_gid_start = 0;
    long node_gid_start = 0;
    int tag = 0;
    int partition_id = 0;
    fillVolumeCells(block, data, cell_gid_start, node_gid_start, tag, partition_id);
    return std::make_shared<TinfMesh>(data, partition_id);
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::create(int num_cells_x,
                                                     int num_cells_y,
                                                     int num_cells_z,
                                                     Parfait::Extent<double> e) {
    Parfait::CartBlock block(e, num_cells_x, num_cells_y, num_cells_z);
    TinfMeshData data;
    long cell_gid_start = 0;
    long node_gid_start = 0;
    int tag = 0;
    int partition_id = 0;
    fillVolumeCells(block, data, cell_gid_start, node_gid_start, tag, partition_id);
    fillSurfaceCells(block, data, cell_gid_start, partition_id);
    auto mesh = std::make_shared<TinfMesh>(data, partition_id);

    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createVolume(
    MessagePasser mp, int nx, int ny, int nz, Parfait::Extent<double> e) {
    std::shared_ptr<inf::TinfMesh> cube = nullptr;
    if (mp.Rank() == 0)
        cube = inf::CartMesh::createVolume(nx, ny, nz, e);
    else
        cube = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());

    return inf::MeshShuffle::repartitionByVolumeCells(mp, *cube);
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::create(
    MessagePasser mp, int nx, int ny, int nz, Parfait::Extent<double> e) {
    std::shared_ptr<inf::TinfMesh> cube = nullptr;
    if (mp.Rank() == 0)
        cube = inf::CartMesh::create(nx, ny, nz, e);
    else
        cube = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());

    return inf::MeshShuffle::repartitionByVolumeCells(mp, *cube);
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSurface(int num_cells_x,
                                                            int num_cells_y,
                                                            int num_cells_z,
                                                            Parfait::Extent<double> e) {
    auto m = create(num_cells_x, num_cells_y, num_cells_z, e);
    auto f = inf::FilterFactory::surfaceFilter(MPI_COMM_SELF, m);
    return std::make_shared<inf::TinfMesh>(f->getMesh());
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSurface(MessagePasser mp,
                                                            int num_cells_x,
                                                            int num_cells_y,
                                                            int num_cells_z,
                                                            Parfait::Extent<double> e) {
    auto m = create(mp, num_cells_x, num_cells_y, num_cells_z, e);
    auto f = inf::FilterFactory::surfaceFilter(MPI_COMM_SELF, m);
    return std::make_shared<inf::TinfMesh>(f->getMesh());
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSphereSurface(MessagePasser mp,
                                                                  int num_cells_x,
                                                                  int num_cells_y,
                                                                  int num_cells_z,
                                                                  Parfait::Extent<double> e) {
    auto m = create(mp, num_cells_x, num_cells_y, num_cells_z, e);
    auto f = inf::FilterFactory::surfaceFilter(MPI_COMM_SELF, m);
    auto mesh = std::make_shared<inf::TinfMesh>(f->getMesh());
    auto& points = mesh->mesh.points;
    for (auto& p : points) {
        double x = p[0];
        double y = p[1];
        double z = p[2];
        p[0] = x * sqrt(1.0 - y * y / 2.0 - z * z / 2.0 + y * y * z * z / 3.0);
        p[1] = y * sqrt(1.0 - x * x / 2.0 - z * z / 2.0 + x * x * z * z / 3.0);
        p[2] = z * sqrt(1.0 - x * x / 2.0 - y * y / 2.0 + x * x * y * y / 3.0);
    }
    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSphereSurface(int num_cells_x,
                                                                  int num_cells_y,
                                                                  int num_cells_z,
                                                                  Parfait::Extent<double> e) {
    auto m = create(num_cells_x, num_cells_y, num_cells_z, e);
    auto f = inf::FilterFactory::surfaceFilter(MPI_COMM_SELF, m);
    auto mesh = std::make_shared<inf::TinfMesh>(f->getMesh());
    auto& points = mesh->mesh.points;
    for (auto& p : points) {
        double x = p[0];
        double y = p[1];
        double z = p[2];
        p[0] = x * sqrt(1.0 - y * y / 2.0 - z * z / 2.0 + y * y * z * z / 3.0);
        p[1] = y * sqrt(1.0 - x * x / 2.0 - z * z / 2.0 + x * x * z * z / 3.0);
        p[2] = z * sqrt(1.0 - x * x / 2.0 - y * y / 2.0 + x * x * y * y / 3.0);
    }
    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSphere(
    MessagePasser mp, int nx, int ny, int nz, Parfait::Extent<double> e) {
    auto mesh = create(mp, nx, ny, nz, e);
    auto& points = mesh->mesh.points;
    for (auto& p : points) {
        double x = p[0];
        double y = p[1];
        double z = p[2];
        p[0] = x * sqrt(1.0 - y * y / 2.0 - z * z / 2.0 + y * y * z * z / 3.0);
        p[1] = y * sqrt(1.0 - x * x / 2.0 - z * z / 2.0 + x * x * z * z / 3.0);
        p[2] = z * sqrt(1.0 - x * x / 2.0 - y * y / 2.0 + x * x * y * y / 3.0);
    }
    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createSphere(int nx,
                                                           int ny,
                                                           int nz,
                                                           Parfait::Extent<double> e) {
    auto mesh = create(nx, ny, nz, e);
    auto& points = mesh->mesh.points;
    for (auto& p : points) {
        double x = p[0];
        double y = p[1];
        double z = p[2];
        p[0] = x * sqrt(1.0 - y * y / 2.0 - z * z / 2.0 + y * y * z * z / 3.0);
        p[1] = y * sqrt(1.0 - x * x / 2.0 - z * z / 2.0 + x * x * z * z / 3.0);
        p[2] = z * sqrt(1.0 - x * x / 2.0 - y * y / 2.0 + x * x * y * y / 3.0);
    }
    return mesh;
}

std::shared_ptr<inf::TinfMesh> inf::CartMesh::createWithBarsAndNodes(int num_cells_x,
                                                                     int num_cells_y,
                                                                     int num_cells_z,
                                                                     Parfait::Extent<double> e) {
    Parfait::CartBlock block(e, num_cells_x, num_cells_y, num_cells_z);
    TinfMeshData data;
    long cell_gid_start = 0;
    long node_gid_start = 0;
    int tag = 0;
    int partition_id = 0;
    fillVolumeCells(block, data, cell_gid_start, node_gid_start, tag, partition_id);
    fillSurfaceCells(block, data, cell_gid_start, partition_id);
    fillEdgeCells(block, data, cell_gid_start);
    fillNodeCells(block, data, cell_gid_start);
    auto mesh = std::make_shared<TinfMesh>(data, partition_id);
    return mesh;
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::createWithBarsAndNodes(
    MessagePasser mp, int nx, int ny, int nz, Parfait::Extent<double> e) {
    std::shared_ptr<inf::TinfMesh> cube = nullptr;
    if (mp.Rank() == 0)
        cube = inf::CartMesh::createWithBarsAndNodes(nx, ny, nz, e);
    else
        cube = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());

    return inf::MeshShuffle::repartitionByVolumeCells(mp, *cube);
}
std::shared_ptr<inf::TinfMesh> inf::CartMesh::create2D(MessagePasser mp,
                                                       int num_x,
                                                       int num_y,
                                                       Parfait::Extent<double, 2> e) {
    std::shared_ptr<inf::TinfMesh> cube = nullptr;
    if (mp.Rank() == 0)
        cube = inf::CartMesh::create2D(num_x, num_y, e);
    else
        cube = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());

    return inf::MeshShuffle::repartitionByVolumeCells(mp, *cube);
}
inf::CartesianBlock::CartesianBlock(const Parfait::Point<double>& lo,
                                    const Parfait::Point<double>& hi,
                                    const std::array<int, 3>& num_cells,
                                    int block_id)
    : lo(lo), num_cells(num_cells), block_id(block_id) {
    dx = (hi[0] - lo[0]) / double(num_cells[0]);
    dy = (hi[1] - lo[1]) / double(num_cells[1]);
    dz = (hi[2] - lo[2]) / double(num_cells[2]);
}
inf::CartesianBlock::CartesianBlock(const std::array<int, 3>& num_cells, int block_id)
    : lo(Parfait::Point<double>{0, 0, 0}), num_cells(num_cells), block_id(block_id) {
    auto hi = Parfait::Point<double>{1, 1, 1};
    dx = (hi[0] - lo[0]) / double(num_cells[0]);
    dy = (hi[1] - lo[1]) / double(num_cells[1]);
    dz = (hi[2] - lo[2]) / double(num_cells[2]);
}
std::array<int, 3> inf::CartesianBlock::nodeDimensions() const {
    return {num_cells[0] + 1, num_cells[1] + 1, num_cells[2] + 1};
}
int inf::CartesianBlock::blockId() const { return block_id; }
Parfait::Point<double> inf::CartesianBlock::point(int i, int j, int k) const {
    Parfait::Point<double> p = lo;
    p[0] += i * dx;
    p[1] += j * dy;
    p[2] += k * dz;
    return p;
}
void inf::CartesianBlock::setPoint(int i, int j, int k, const Parfait::Point<double>& p) {
    PARFAIT_THROW("CartesianBlock does not support changing point locations");
}
