#pragma once
#include "TinfMesh.h"
#include "MeshBuilder.h"

namespace inf {
namespace extrude {
    enum Type { AXISYMMETRIC_X = 1 };

    std::shared_ptr<TinfMesh> extrudeAxisymmetric(MessagePasser mp,
                                                  const MeshInterface& m,
                                                  Type type,
                                                  double angle_in_degrees,
                                                  bool symmetric = false);

    std::shared_ptr<TinfMesh> extrudeTags(const MeshInterface& m,
                                          std::set<int> tags,
                                          double spacing,
                                          int nlayers,
                                          bool both_sides = false,
                                          int new_tag = -12);
    void extrudeTags(inf::experimental::MeshBuilder& builder,
                     std::set<int> tags_to_extrude,
                     bool erase_extruded_surfaces,
                     bool extrude_reverse,
                     double spacing,
                     int new_tag = -98);

    struct Cell {
        bool is_null = false;
        inf::MeshInterface::CellType type;
        std::vector<int> cell_nodes = {};
    };

    Cell extrudeTriangle(std::vector<int> cell_nodes, const std::vector<int>& old_to_new);
    Cell extrudeBar(std::vector<int> cell_nodes, const std::vector<int>& old_to_new);
}
}
