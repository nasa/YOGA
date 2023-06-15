#pragma once
#include <array>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>

namespace inf  {
namespace mock {

    inline std::shared_ptr<inf::MeshInterface> getSingleTetMesh(int partition_id = 0) {
        inf::TinfMeshData mesh_data;
        mesh_data.cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
        mesh_data.cells[inf::MeshInterface::TRI_3] = {0, 2, 1, 0, 1, 3, 1, 2, 3, 0, 3, 2};
        mesh_data.cell_tags[inf::MeshInterface::TRI_3] = {0, 0, 0, 0};
        mesh_data.cell_tags[inf::MeshInterface::TETRA_4] = {0};
        mesh_data.global_cell_id[inf::MeshInterface::TRI_3] = {1, 2, 3, 4};
        mesh_data.global_cell_id[inf::MeshInterface::TETRA_4] = {0};
        mesh_data.cell_owner[inf::MeshInterface::TRI_3] = {0, 0, 0, 0};
        mesh_data.cell_owner[inf::MeshInterface::TETRA_4] = {0};
        mesh_data.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        mesh_data.node_owner = {0, 0, 0, 0};
        mesh_data.global_node_id = {0, 1, 2, 3};

        return std::make_shared<inf::TinfMesh>(mesh_data, partition_id);
    }

    inline std::shared_ptr<inf::MeshInterface> getSingleHexMesh(int partition_id = 0) {
        inf::TinfMeshData mesh_data;
        mesh_data.cells[inf::MeshInterface::HEXA_8] = {0, 1, 2, 3, 4, 5, 6, 7};
        mesh_data.cell_tags[inf::MeshInterface::HEXA_8] = {0};
        mesh_data.global_cell_id[inf::MeshInterface::HEXA_8] = {0};
        mesh_data.cell_owner[inf::MeshInterface::HEXA_8] = {partition_id};
        mesh_data.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
        int num_points = 8;
        mesh_data.node_owner = std::vector<int>(num_points, partition_id);
        mesh_data.global_node_id.resize(num_points);
        std::iota(mesh_data.global_node_id.begin(), mesh_data.global_node_id.begin() + num_points, 0);

        return std::make_shared<inf::TinfMesh>(mesh_data, partition_id);
    }

inline std::shared_ptr<inf::TinfMesh> getSingleHexMeshWithFaces(int partition_id = 0) {
    inf::TinfMeshData mesh_data;
    mesh_data.cells[inf::MeshInterface::HEXA_8] = {0, 1, 2, 3, 4, 5, 6, 7};
    mesh_data.cells[inf::MeshInterface::QUAD_4] = {0,3,2,1,0,1,5,4,1,2,6,5,2,3,7,6,0,4,7,3,4,5,6,7};
    mesh_data.cell_tags[inf::MeshInterface::HEXA_8] = {0};
    mesh_data.cell_tags[inf::MeshInterface::QUAD_4] = {1,2,3,4,5,6};
    mesh_data.global_cell_id[inf::MeshInterface::HEXA_8] = {0};
    mesh_data.global_cell_id[inf::MeshInterface::QUAD_4] = {1,2,3,4,5,6};
    mesh_data.cell_owner[inf::MeshInterface::HEXA_8] = {partition_id};
    mesh_data.cell_owner[inf::MeshInterface::QUAD_4] = std::vector<int>(6, partition_id);
    mesh_data.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    int num_points = 8;
    mesh_data.node_owner = std::vector<int>(num_points, partition_id);
    mesh_data.global_node_id.resize(num_points);
    std::iota(mesh_data.global_node_id.begin(), mesh_data.global_node_id.begin() + num_points, 0);

    return std::make_shared<inf::TinfMesh>(mesh_data, partition_id);
}

inline std::shared_ptr<inf::MeshInterface> getSingleTetMeshNoNeighbors() {
        inf::TinfMeshData mesh_data;
        mesh_data.cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
        mesh_data.cell_tags[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
        mesh_data.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        mesh_data.node_owner = {0, 0, 0, 0};
        mesh_data.global_node_id = {0, 1, 2, 3};

        int partition_id = 0;
        return std::make_shared<inf::TinfMesh>(mesh_data, partition_id);
    }

    inline std::shared_ptr<inf::TinfMesh> threeManifoldTriangles() {
        inf::TinfMeshData mesh_data;
        mesh_data.cells[inf::MeshInterface::TRI_3] = {0, 2, 1, 0,1,3,1,2,3,2,0,3};
        mesh_data.cell_tags[inf::MeshInterface::TRI_3] = {0, 1, 2, 3};
        mesh_data.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        mesh_data.node_owner = {0, 0, 0, 0};
        mesh_data.global_node_id = {0, 1, 2, 3};

        int partition_id = 0;
        return std::make_shared<inf::TinfMesh>(mesh_data, partition_id);
    }

    inline TinfMeshData onePrism() {
        TinfMeshData mesh;
        mesh.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
        mesh.cells[inf::MeshInterface::PENTA_6] = {0, 1, 2, 3, 4, 5};
        mesh.global_cell_id[inf::MeshInterface::PENTA_6] = {0};
        mesh.cell_owner[inf::MeshInterface::PENTA_6] = {0};
        mesh.cell_tags[inf::MeshInterface::PENTA_6] = {0};
        mesh.node_owner = {0, 0, 0, 0, 0, 0};
        mesh.global_node_id = {0, 1, 2, 3, 4, 5};
        return mesh;
    }

inline TinfMeshData onePyramid() {
    TinfMeshData mesh;
    mesh.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}};
    mesh.cells[inf::MeshInterface::PYRA_5] = {0, 1, 2, 3, 4};
    mesh.global_cell_id[inf::MeshInterface::PYRA_5] = {0};
    mesh.cell_owner[inf::MeshInterface::PYRA_5] = {0};
    mesh.cell_tags[inf::MeshInterface::PYRA_5] = {0};
    mesh.node_owner = {0, 0, 0, 0, 0};
    mesh.global_node_id = {0, 1, 2, 3, 4};
    return mesh;
}

inline TinfMeshData oneTet() {
    TinfMeshData mesh;
    mesh.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    mesh.cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
    mesh.global_cell_id[inf::MeshInterface::TETRA_4] = {0};
    mesh.cell_owner[inf::MeshInterface::TETRA_4] = {0};
    mesh.cell_tags[inf::MeshInterface::TETRA_4] = {0};
    mesh.node_owner = {0, 0, 0, 0};
    mesh.global_node_id = {0, 1, 2, 3};
    return mesh;
}

inline TinfMeshData oneTriangle() {
    TinfMeshData mesh;
        mesh.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
        mesh.cells[inf::MeshInterface::TRI_3] = {0, 1, 2};
        mesh.global_cell_id[inf::MeshInterface::TRI_3] = {0};
        mesh.cell_owner[inf::MeshInterface::TRI_3] = {0};
        mesh.cell_tags[inf::MeshInterface::TRI_3] = {0};
        mesh.node_owner = {0, 0, 0};
        mesh.global_node_id = {0, 1, 2};
        return mesh;
    }

inline TinfMeshData oneQuad() {
    TinfMeshData mesh;
    mesh.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0,1,0}};
    mesh.cells[inf::MeshInterface::QUAD_4] = {0, 1, 2, 3};
    mesh.global_cell_id[inf::MeshInterface::QUAD_4] = {0};
    mesh.cell_owner[inf::MeshInterface::QUAD_4] = {0};
    mesh.cell_tags[inf::MeshInterface::QUAD_4] = {0};
    mesh.node_owner = {0, 0, 0, 0};
    mesh.global_node_id = {0, 1, 2, 3};
    return mesh;
}

inline TinfMeshData triangleWithBar() {
    auto mesh = oneTriangle();
    mesh.cells[inf::MeshInterface::BAR_2] = {0, 1};
    mesh.global_cell_id[inf::MeshInterface::BAR_2] = {3};
    mesh.cell_owner[inf::MeshInterface::BAR_2] = {0};
    mesh.cell_tags[inf::MeshInterface::BAR_2] = {1};
    return mesh;
}

inline TinfMeshData hangingEdge() {

    inf::TinfMeshData mesh_data;
    mesh_data.cells[inf::MeshInterface::HEXA_8] = {0, 1, 2, 3, 4, 5, 6, 7};
    mesh_data.cells[inf::MeshInterface::PENTA_6] = {4, 5, 7, 8, 9, 11, 5, 6, 7, 9, 10, 11};
    mesh_data.cells[inf::MeshInterface::QUAD_4] = {0,3,2,1, 0,1,5,4, 0,4,7,3, 1,2,6,5, 2,3,7,6, 5,6,10,9, 6,7,11,10, 4,8,11,7, 4,5,9,8};
    mesh_data.cells[inf::MeshInterface::TRI_3] = {8,9,11,9,10,11};

    mesh_data.cell_tags[inf::MeshInterface::HEXA_8] = {0};
    mesh_data.global_cell_id[inf::MeshInterface::HEXA_8] = {0};
    mesh_data.cell_owner[inf::MeshInterface::HEXA_8] = {0};

    mesh_data.cell_tags[inf::MeshInterface::QUAD_4].resize(9,0);
    mesh_data.global_cell_id[inf::MeshInterface::QUAD_4] = {3,4,5,6,7,8,9,10,11};
    mesh_data.cell_owner[inf::MeshInterface::QUAD_4].resize(9,0);

    mesh_data.cell_tags[inf::MeshInterface::TRI_3] = {0,0};
    mesh_data.global_cell_id[inf::MeshInterface::TRI_3] = {12,13};
    mesh_data.cell_owner[inf::MeshInterface::TRI_3] = {0,0};

    mesh_data.global_cell_id[inf::MeshInterface::HEXA_8] = {0};
    mesh_data.cell_owner[inf::MeshInterface::HEXA_8] = {0};

    mesh_data.cell_tags[inf::MeshInterface::PENTA_6] = {0, 0};
    mesh_data.global_cell_id[inf::MeshInterface::PENTA_6] = {1, 2};
    mesh_data.cell_owner[inf::MeshInterface::PENTA_6] = {0, 0};

    mesh_data.points = {
        {0,0,0},{1,0,0},{1,1,0},{0,1,0},
        {0,0,1},{1,0,1},{1,1,1},{0,1,1},
        {0,0,2},{1,0,2},{1,1,2},{0,1,2}};

    mesh_data.node_owner.resize(12, 0);
    mesh_data.global_node_id = {0,1,2,3,4,5,6,7,8,9,10,11};
    return mesh_data;
}

inline TinfMeshData twoTouchingTets() {
        TinfMeshData mesh;
        mesh.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
        mesh.cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3, 3, 2, 1, 4};
        mesh.global_cell_id[inf::MeshInterface::TETRA_4] = {0, 1};
        mesh.cell_owner[inf::MeshInterface::TETRA_4] = {0, 0};
        mesh.cell_tags[inf::MeshInterface::TETRA_4] = {0, 0};
        mesh.node_owner = {0, 0, 0, 0, 0};
        mesh.global_node_id = {0, 1, 2, 3, 4};
        return mesh;
    }

    class TwoTetMesh : public MeshInterface {
      public:
        inline TwoTetMesh() {
            points.resize(8);
            points[0] = {0, 0, 0};
            points[1] = {1, 0, 0};
            points[2] = {0, 1, 0};
            points[3] = {0, 0, 1};

            points[4] = {0, 0, 10};
            points[5] = {1, 0, 10};
            points[6] = {0, 1, 10};
            points[7] = {0, 0, 11};
        }

        inline int nodeCount() const override { return 8; }

        inline int cellCount() const override { return 2; }

        inline int partitionId() const override { return 0; }

        inline void nodeCoordinate(int node_id, double* p) const override {
            p[0] = points[node_id][0];
            p[1] = points[node_id][1];
            p[2] = points[node_id][2];
        }

        inline MeshInterface::CellType cellType(int cell_id) const override { return MeshInterface::TETRA_4; }

        inline int cellCount(MeshInterface::CellType cell_type) const override {
            if (cell_type == MeshInterface::TETRA_4) return 2;
            return 0;
        }

        inline void cell(int cell_id, int* c) const override {
            if (cell_id == 0) {
                c[0] = 0;
                c[1] = 1;
                c[2] = 2;
                c[3] = 3;
            } else {
                c[0] = 4;
                c[1] = 5;
                c[2] = 6;
                c[3] = 7;
            }
        }

        inline long globalNodeId(int node_id) const override { return node_id; }

        inline long globalCellId(int cell_id) const override { return cell_id; }

        inline int cellOwner(int cell_id) const override { return 0; }

        inline int cellTag(int cell_id) const override { return cell_id; }

        inline int nodeOwner(int node_id) const override { return 0; }

      private:
        std::vector<std::array<double, 3>> points;
    };
}
}
