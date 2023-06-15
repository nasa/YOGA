#pragma once
#include "MeshInterface.h"

namespace inf {
class MeshMemoryConsumption {
  public:
    inline MeshMemoryConsumption()
        : xyz_size_bytes(0),
          c2n_size_bytes(0),
          tag_size_bytes(0),
          node_gid_size_bytes(0),
          cell_gid_size_bytes(0),
          node_owner_size_bytes(0),
          cell_owner_size_bytes(0),
          total(0),
          owned_total(0),
          ghost_total(0) {}
    inline static MeshMemoryConsumption calc(const inf::MeshInterface& mesh) {
        MeshMemoryConsumption m;
        m.xyz_size_bytes = mesh.nodeCount() * 3 * sizeof(double);

        for (int c = 0; c < mesh.cellCount(); c++) {
            auto length = mesh.cellLength(c);
            m.c2n_size_bytes += length * sizeof(int);
        }

        m.node_gid_size_bytes = sizeof(long) * mesh.nodeCount();
        m.cell_gid_size_bytes = sizeof(long) * mesh.cellCount();
        m.tag_size_bytes = sizeof(int) * mesh.cellCount();
        m.node_owner_size_bytes = sizeof(int) * mesh.nodeCount();
        m.cell_owner_size_bytes = sizeof(int) * mesh.cellCount();

        //                        xyz             gid           owner
        size_t node_cost = 3 * sizeof(double) + sizeof(long) + sizeof(int);
        //                        gid             owner         tag
        size_t cell_base_cost = sizeof(long) + sizeof(int) + sizeof(int);

        for (int n = 0; n < mesh.nodeCount(); n++) {
            if (mesh.ownedNode(n)) {
                m.owned_total += node_cost;
            } else {
                m.ghost_total += node_cost;
            }
        }

        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.ownedCell(c)) {
                m.owned_total += cell_base_cost + sizeof(int) * mesh.cellLength(c);
            } else {
                m.ghost_total += cell_base_cost + sizeof(int) * mesh.cellLength(c);
            }
        }

        m.total = m.ghost_total + m.owned_total;

        return m;
    }

    inline long totalInMB() { return total / 1024 / 1024; }

    inline long ownedInMB() { return owned_total / 1024 / 1024; }

    inline long ghostInMB() { return ghost_total / 1024 / 1024; }

  public:
    size_t xyz_size_bytes;
    size_t c2n_size_bytes;
    size_t tag_size_bytes;
    size_t node_gid_size_bytes;
    size_t cell_gid_size_bytes;
    size_t node_owner_size_bytes;
    size_t cell_owner_size_bytes;

    size_t total;
    size_t owned_total;
    size_t ghost_total;
};
}
