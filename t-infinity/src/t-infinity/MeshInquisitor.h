#pragma once
#include <t-infinity/MeshInterface.h>
#include <parfait/Point.h>
#include <parfait/Metrics.h>
#include <parfait/CGNSElements.h>
#include <parfait/CGNSFaceExtraction.h>

namespace inf {

class MeshInquisitor {
  public:
    MeshInquisitor(const MeshInterface& m) : mesh(m) {}

    const std::vector<int>& cell(int id) {
        mesh.cell(id, node_ids);
        return node_ids;
    }

    const std::vector<Parfait::Point<double>>& vertices(int cell_id) {
        setCellPoints(cell_id);
        return points;
    }
    double cellArea(int cell_id) {
        setCellPoints(cell_id);
        auto type = mesh.cellType(cell_id);
        double v = 0.0;
        if (MeshInterface::TRI_3 == type) {
            v = Parfait::Metrics::computeTriangleArea(points[0], points[1], points[2]).magnitude();
        } else if (MeshInterface::QUAD_4 == type) {
            auto area = Parfait::Metrics::computeTriangleArea(points[0], points[1], points[2]) +
                        Parfait::Metrics::computeTriangleArea(points[2], points[3], points[0]);
            v = area.magnitude();
        } else {
            throw std::logic_error("Cell type: " + std::to_string(type) +
                                   " not supported for area calculation");
        }
        return v;
    }

    double cellVolume(int cell_id) {
        setCellPoints(cell_id);
        auto type = mesh.cellType(cell_id);
        if (MeshInterface::TETRA_4 == type)
            return Parfait::Metrics::computeTetVolume(points[0], points[1], points[2], points[3]);
        auto centroid = Parfait::Metrics::average(points);
        double v = 0.0;
        if (MeshInterface::PYRA_5 == type) {
            for (int i = 0; i < 5; i++) {
                Parfait::CGNS::getPyramidFace(points, i, face_points);
                v += calcFaceVolumeContribution(centroid);
            }
        } else if (MeshInterface::PENTA_6 == type) {
            for (int i = 0; i < 5; i++) {
                Parfait::CGNS::getPrismFace(points, i, face_points);
                v += calcFaceVolumeContribution(centroid);
            }
        } else if (MeshInterface::HEXA_8 == type) {
            for (int i = 0; i < 6; i++) {
                Parfait::CGNS::getHexFace(points, i, face_points);
                v += calcFaceVolumeContribution(centroid);
            }
        } else {
            throw std::logic_error("Cell type: " + std::to_string(type) +
                                   " not supported for volume calculation");
            return 0.0;
        }
        return v;
    }

  private:
    const MeshInterface& mesh;
    std::vector<int> node_ids;
    std::vector<Parfait::Point<double>> points;
    std::vector<Parfait::Point<double>> face_points;

    void setCellPoints(int cell_id) {
        mesh.cell(cell_id, node_ids);
        points.resize(node_ids.size());
        for (size_t i = 0; i < node_ids.size(); i++) points[i] = mesh.node(node_ids[i]);
    }

    double calcFaceVolumeContribution(const Parfait::Point<double>& centroid) {
        double v = Parfait::Metrics::computeTetVolume(
            face_points[0], face_points[2], face_points[1], centroid);
        if (face_points.size() == 4)
            v += Parfait::Metrics::computeTetVolume(
                face_points[0], face_points[3], face_points[2], centroid);
        return v;
    }
};
}
