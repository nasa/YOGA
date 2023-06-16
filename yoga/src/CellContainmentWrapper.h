#pragma once
#include "YogaMesh.h"
#include <parfait/CellContainmentChecker.h>

namespace YOGA {

class CellContainmentWrapper {
  public:
    static bool contains(const YogaMesh& mesh,int cell_id, double x, double y, double z){
        int n = mesh.numberOfNodesInCell(cell_id);
        switch(n){
            case 4:
                return buildAndCheckCell<4>(mesh,cell_id,x,y,z);
            case 5:
                return buildAndCheckCell<5>(mesh,cell_id,x,y,z);
            case 6:
                return buildAndCheckCell<6>(mesh,cell_id,x,y,z);
            case 8:
                return buildAndCheckCell<8>(mesh,cell_id,x,y,z);
            default:
                throw std::logic_error("Can't check containment of cell with size: "+std::to_string(n));
        }
    }
  private:

    template <int N>
    static bool buildAndCheckCell(const YogaMesh& mesh, int cell_id, double x, double y, double z) {
        std::array<int, N> cell;
        mesh.getNodesInCell(cell_id, cell.data());
        std::array<Parfait::Point<double>, N> vertices;
        for (int i = 0; i < N; i++) {
            vertices[i] = mesh.getNode<double>(cell[i]);
        }
        return Parfait::CellContainmentChecker::isInCell<N>(vertices, {x, y, z});
    }
};
}