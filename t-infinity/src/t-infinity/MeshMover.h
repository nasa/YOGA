#pragma once
#include <t-infinity/TinfMesh.h>
#include <parfait/MotionMatrix.h>
#include <parfait/RewindLeftHandedElements.h>

namespace inf {
class MeshMover {
  public:
    static std::shared_ptr<MeshInterface> move(std::shared_ptr<MeshInterface> mesh,
                                               Parfait::MotionMatrix motion) {
        auto m = std::make_shared<inf::TinfMesh>(mesh);
        Parfait::Point<double> p;
        for (int i = 0; i < m->nodeCount(); i++) {
            p = m->node(i);
            motion.movePoint(p.data());
            m->setNodeCoordinate(i, p.data());
        }
        return m;
    }
    static std::shared_ptr<MeshInterface> scale(std::shared_ptr<MeshInterface> mesh,
                                                double scaling) {
        auto m = std::make_shared<inf::TinfMesh>(mesh);
        Parfait::Point<double> p;
        for (int i = 0; i < m->nodeCount(); i++) {
            p = m->node(i);
            p *= scaling;
            m->setNodeCoordinate(i, p.data());
        }
        return m;
    }
    static std::shared_ptr<MeshInterface> reflect(std::shared_ptr<MeshInterface> mesh,
                                                  const Parfait::Point<double>& plane_normal,
                                                  double plane_offset) {
        auto m = std::make_shared<inf::TinfMesh>(mesh);
        reflectPoints(*m, plane_normal, plane_offset);

        auto& mesh_data = m->mesh;
        rewindTets(mesh_data);
        rewindPyramids(mesh_data);
        rewindPrisms(mesh_data);
        rewindHexs(mesh_data);
        rewindTriangles(mesh_data);
        rewindQuads(mesh_data);

        return m;
    }

    static Parfait::Point<double> reflect(const Parfait::Point<double>& p,
                                          const Parfait::Point<double>& plane_normal,
                                          double offset) {
        double a = plane_normal[0];
        double b = plane_normal[1];
        double c = plane_normal[2];
        double d = offset;
        double distance_to_plane =
            (a * p[0] + b * p[1] + c * p[2] + d) / std::sqrt(a * a + b * b + c * c);
        return p - 2.0 * distance_to_plane * plane_normal;
    }

  private:
    static void reflectPoints(TinfMesh& m, const Parfait::Point<double>& plane, double offset) {
        for (int i = 0; i < m.nodeCount(); i++) {
            Parfait::Point<double> p = m.node(i);
            p = reflect(p, plane, offset);
            m.setNodeCoordinate(i, p.data());
        }
    }

    template <typename Rewinder>
    static void rewind(const std::vector<int>& cells, Rewinder rewinder, int cell_size) {
        auto N = int(cells.size()) / cell_size;
        for (int i = 0; i < N; i++) {
            auto cell = (int*)cells.data() + cell_size * i;
            rewinder(cell);
        }
    }

    static void rewindTriangles(TinfMeshData& m) {
        auto& triangles = m.cells[MeshInterface::CellType::TRI_3];
        rewind(triangles, &Parfait::rewindLeftHandedTriangle<int*>, 3);
    }

    static void rewindQuads(TinfMeshData& m) {
        auto& quads = m.cells[MeshInterface::CellType::QUAD_4];
        rewind(quads, &Parfait::rewindLeftHandedQuad<int*>, 4);
    }

    static void rewindTets(TinfMeshData& m) {
        auto& tets = m.cells[MeshInterface::CellType::TETRA_4];
        rewind(tets, &Parfait::rewindLeftHandedTet<int*>, 4);
    }

    static void rewindPyramids(TinfMeshData& m) {
        auto& pyramids = m.cells[MeshInterface::CellType::PYRA_5];
        rewind(pyramids, &Parfait::rewindLeftHandedPyramid<int*>, 5);
    }

    static void rewindPrisms(TinfMeshData& m) {
        auto& prisms = m.cells[MeshInterface::CellType::PENTA_6];
        rewind(prisms, &Parfait::rewindLeftHandedPrism<int*>, 6);
    }

    static void rewindHexs(TinfMeshData& m) {
        auto& hexs = m.cells[MeshInterface::CellType::HEXA_8];
        rewind(hexs, &Parfait::rewindLeftHandedHex<int*>, 8);
    }
};
}