#include "DiagonalTetsMockMesh.h"
#include "BoundaryConditions.h"
namespace YOGA {
YogaMesh generateDiagonalTetsMockMesh(int rank) {
    YogaMesh mesh;
    mesh.setFaceCount(4);
    auto get_face = [](int id, int* face) {
        if (0 == id) {
            face[0] = 0;
            face[1] = 2;
            face[2] = 1;
        } else if (1 == id) {
            face[0] = 1;
            face[1] = 2;
            face[2] = 3;
        } else if (2 == id) {
            face[0] = 2;
            face[1] = 0;
            face[2] = 3;
        } else {
            face[0] = 0;
            face[1] = 1;
            face[2] = 3;
        }
    };
    mesh.setFaces([](int) { return 3; }, get_face);
    mesh.setBoundaryConditions([](int) { return YOGA::BoundaryConditions::Solid; }, [](int){return 3;});
    mesh.setNodeCount(4);
    auto get_xyz_for_node = [=](int id, double* xyz) {
        for (int i = 0; i < 3; i++) xyz[i] = rank;
        if (1 == id) ++xyz[0];
        if (2 == id) ++xyz[1];
        if (3 == id) ++xyz[2];
    };
    mesh.setXyzForNodes(get_xyz_for_node);
    mesh.setCellCount(1);
    mesh.setCells([](int) { return 4; },
                  [](int, int* cell) {
                      for (int i = 0; i < 4; i++) cell[i] = i;
                  });
    return mesh;
}
}