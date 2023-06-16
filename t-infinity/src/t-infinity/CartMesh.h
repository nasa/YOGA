#pragma once
#include <array>
#include <memory>
#include <parfait/Extent.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/StructuredMesh.h>
#include <t-infinity/TinfMesh.h>

namespace inf {
namespace CartMesh {

    std::shared_ptr<inf::TinfMesh> create(MessagePasser mp,
                                          int nx,
                                          int ny,
                                          int nz,
                                          Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> create(int num_cells_x,
                                     int num_cells_y,
                                     int num_cells_z,
                                     Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> createVolume(MessagePasser mp,
                                           int num_cells_x,
                                           int num_cells_y,
                                           int num_cells_z,
                                           Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> createVolume(int num_cells_x,
                                           int num_cells_y,
                                           int num_cells_z,
                                           Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});

    std::shared_ptr<TinfMesh> createSphere(MessagePasser mp,
                                           int num_cells_x,
                                           int num_cells_y,
                                           int num_cells_z,
                                           Parfait::Extent<double> e = {{-1, -1, -1}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> createSphere(int num_cells_x,
                                           int num_cells_y,
                                           int num_cells_z,
                                           Parfait::Extent<double> e = {{-1, -1, -1}, {1, 1, 1}});

    std::shared_ptr<TinfMesh> createSphereSurface(MessagePasser mp,
                                                  int num_cells_x,
                                                  int num_cells_y,
                                                  int num_cells_z,
                                                  Parfait::Extent<double> e = {{-1, -1, -1},
                                                                               {1, 1, 1}});
    std::shared_ptr<TinfMesh> createSphereSurface(int num_cells_x,
                                                  int num_cells_y,
                                                  int num_cells_z,
                                                  Parfait::Extent<double> e = {{-1, -1, -1},
                                                                               {1, 1, 1}});

    std::shared_ptr<TinfMesh> createSurface(MessagePasser mp,
                                            int num_cells_x,
                                            int num_cells_y,
                                            int num_cells_z,
                                            Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> createSurface(int num_cells_x,
                                            int num_cells_y,
                                            int num_cells_z,
                                            Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}});
    std::shared_ptr<TinfMesh> createWithBarsAndNodes(MessagePasser mp,
                                                     int num_cells_x,
                                                     int num_cells_y,
                                                     int num_cells_z,
                                                     Parfait::Extent<double> e = {{0, 0, 0},
                                                                                  {1, 1, 1}});
    std::shared_ptr<TinfMesh> createWithBarsAndNodes(int num_cells_x,
                                                     int num_cells_y,
                                                     int num_cells_z,
                                                     Parfait::Extent<double> e = {{0, 0, 0},
                                                                                  {1, 1, 1}});
    std::shared_ptr<TinfMesh> create2D(MessagePasser mp,
                                       int num_x,
                                       int num_y,
                                       Parfait::Extent<double, 2> e = {{0, 0}, {1, 1}});
    std::shared_ptr<TinfMesh> create2D(int num_x,
                                       int num_y,
                                       Parfait::Extent<double, 2> e = {{0, 0}, {1, 1}});
    std::shared_ptr<TinfMesh> create2DWithBarsAndNodes(
        int num_x, int num_y, Parfait::Extent<double, 2> e = {{0, 0}, {1, 1}});
    std::shared_ptr<TinfMesh> create2DWithBars(int num_x,
                                               int num_y,
                                               Parfait::Extent<double, 2> e = {{0, 0}, {1, 1}});

}

class CartesianBlock : public inf::StructuredBlock {
  public:
    CartesianBlock(const Parfait::Point<double>& lo,
                   const Parfait::Point<double>& hi,
                   const std::array<int, 3>& num_cells,
                   int block_id = 0);

    CartesianBlock(const std::array<int, 3>& num_cells, int block_id = 0);

    std::array<int, 3> nodeDimensions() const override;

    int blockId() const override;

    Parfait::Point<double> point(int i, int j, int k) const override;
    void setPoint(int i, int j, int k, const Parfait::Point<double>& p) override;

  private:
    Parfait::Point<double> lo;
    std::array<int, 3> num_cells;
    double dx, dy, dz;
    int block_id;
};
}
