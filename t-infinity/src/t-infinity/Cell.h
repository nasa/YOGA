#pragma once
#include <vector>
#include <parfait/Point.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <parfait/Extent.h>

namespace inf {
class Cell {
  public:
    explicit Cell(const inf::MeshInterface& mesh, int cell_id, int mesh_dimension = 3);

    explicit Cell(const inf::TinfMesh& mesh,
                  MeshInterface::CellType type,
                  int cell_index_in_type,
                  int mesh_dimension = 3);
    explicit Cell(std::shared_ptr<inf::MeshInterface> mesh, int cell_id, int mesh_dimension = 3);

    explicit Cell(MeshInterface::CellType type,
                  const std::vector<int>& cell_nodes,
                  const std::vector<Parfait::Point<double>>& cell_points,
                  int mesh_dimension = 3);
    explicit Cell(MeshInterface::CellType type,
                  const int* cell_nodes,
                  const Parfait::Point<double>* cell_points,
                  int mesh_dimension = 3);

    inf::MeshInterface::CellType type() const;
    std::vector<int> nodes() const;
    void nodes(int* cell_nodes) const;
    int faceCount() const;
    int faceCount2D() const;
    int edgeCount() const;
    double volume() const;
    Parfait::Extent<double> extent() const;
    Parfait::Point<double> averageCenter() const;
    Parfait::Point<double> nishikawaCentroid(double p = 2.0) const;
    Parfait::Point<double> faceAverageCenter(int face_number) const;
    Parfait::Point<double> faceAreaNormal(int face_number) const;
    Parfait::Point<double> faceAreaNormal2D(int face_number) const;
    std::vector<int> faceNodes(int face_number) const;
    int boundingEntityCount() const;
    std::vector<int> nodesInBoundingEntity(int id) const;
    std::vector<int> faceNodes2D(int face_number) const;
    std::vector<std::array<int, 2>> edges() const;
    static std::vector<int> faceNodes(int face_number,
                                      MeshInterface::CellType cell_type,
                                      const std::vector<int>& cell_nodes,
                                      int dimension_mode);
    std::vector<Parfait::Point<double>> facePoints(int face_number) const;
    std::vector<Parfait::Point<double>> facePoints2D(int face_number) const;
    std::vector<Parfait::Point<double>> points() const;
    void points(std::vector<Parfait::Point<double>>& cell_points) const;
    int dimension() const;
    void visualize(std::string filename) const;

    bool contains(Parfait::Point<double> p);

  private:
    int dimension_mode = 3;
    inf::MeshInterface::CellType cell_type;
    std::vector<int> cell_nodes;
    std::vector<Parfait::Point<double>> cell_points;
    double area2D() const;
};
}
