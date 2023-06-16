#pragma once

#include <parfait/Throw.h>
#include <parfait/LeastSquaresReconstruction.h>
#include <parfait/ToString.h>
#include <t-infinity/BoundaryNodes.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <parfait/SyncPattern.h>
#include <parfait/SyncField.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/GlobalToLocal.h>

namespace inf {

class NodeToNodeGradientCalculator {
  public:
    inline NodeToNodeGradientCalculator(MessagePasser mp, std::shared_ptr<inf::MeshInterface> mesh)
        : mp(mp),
          mesh(mesh),
          node_stencils(inf::NodeToNode::buildForAnyNodeInSharedCell(*mesh)),
          node_stencil_coeffs(mesh->nodeCount()),
          node_sync_pattern(inf::buildNodeSyncPattern(mp, *mesh)),
          node_g2l(inf::GlobalToLocal::buildNode(*mesh)) {
        for (int n = 0; n < mesh->nodeCount(); n++) {
            node_stencils.at(n).push_back(n);
        }

        auto calc_node_weights = [&](int n) {
            if (mesh->nodeOwner(n) != mp.Rank()) {
                node_stencils.at(n).resize(0);  // zero out this row's stencil.
                return;
            }
            int num_neighbors = node_stencils.at(n).size();
            node_stencil_coeffs.at(n).resize(num_neighbors);
            auto get_neighbor_location = [&](int neighbor) {
                int c = node_stencils.at(n)[neighbor];
                Parfait::Point<double> p = mesh->node(c);
                return p;
            };
            double distance_weight_power = 0.0;
            Parfait::Point<double> node_location = mesh->node(n);
            auto dimension = Parfait::detectStencilDimension(get_neighbor_location, num_neighbors);
            auto row_coeffs = Parfait::calcLLSQCoefficients(get_neighbor_location,
                                                            num_neighbors,
                                                            distance_weight_power,
                                                            node_location,
                                                            Parfait::LSQDimensionality(dimension));
            for (int i = 0; i < num_neighbors; i++) {
                node_stencil_coeffs[n][i][0] = row_coeffs(i, 1);
                node_stencil_coeffs[n][i][1] = row_coeffs(i, 2);
                if (dimension > 2)
                    node_stencil_coeffs[n][i][2] = row_coeffs(i, 3);
                else
                    node_stencil_coeffs[n][i][2] = 0.0;
            }
        };

        for (int n = 0; n < mesh->nodeCount(); n++) {
            calc_node_weights(n);
        }
    }
    inline std::vector<Parfait::Point<double>> calcGrad(const inf::FieldInterface& f) const {
        if (f.size() != mesh->nodeCount() or f.association() != inf::FieldAttributes::Node()) {
            PARFAIT_THROW(
                "Cannot compute cell to node gradient if field is not defined at cells with "
                "matching length");
        }
        auto f_accessor = [&f](int n) { return extractValue(n, f); };
        return calcGrad(f_accessor);
    }

    inline std::vector<std::array<double, 6>> calcHessian(const inf::FieldInterface& f) const {
        if (f.size() != mesh->nodeCount() or f.association() != inf::FieldAttributes::Node()) {
            PARFAIT_THROW(
                "Cannot compute cell to node gradient if field is not defined at cells with "
                "matching length");
        }
        auto f_accessor = [&f](int n) { return extractValue(n, f); };
        return calcHessian(f_accessor);
    }

    template <typename FieldAccessor>
    std::vector<Parfait::Point<double>> calcGrad(FieldAccessor f) const {
        std::vector<Parfait::Point<double>> grad(mesh->nodeCount(),
                                                 Parfait::Point<double>(0, 0, 0));

        for (int n = 0; n < mesh->nodeCount(); n++) {
            for (int i = 0; i < int(node_stencils.at(n).size()); i++) {
                PARFAIT_ASSERT(node_stencils.at(n).size() == node_stencil_coeffs.at(n).size(),
                               "stencil coeff size mismatch");
                for (int e = 0; e < 3; e++) {
                    grad[n][e] += node_stencil_coeffs.at(n).at(i)[e] * f(node_stencils.at(n)[i]);
                }
            }
        }
        Parfait::syncVector(mp, grad, node_g2l, node_sync_pattern);
        return grad;
    }

    template <typename FieldAccessor>
    std::vector<std::array<double, 6>> calcHessian(FieldAccessor f) const {
        auto grad = calcGrad(f);
        std::vector<std::array<double, 6>> hess(mesh->nodeCount());

        auto f_x = [&](int n) { return grad[n][0]; };
        auto f_y = [&](int n) { return grad[n][1]; };
        auto f_z = [&](int n) { return grad[n][2]; };

        auto grad_f_x = calcGrad(f_x);
        auto grad_f_y = calcGrad(f_y);
        auto grad_f_z = calcGrad(f_z);

        for (int node = 0; node < mesh->nodeCount(); node++) {
            double hess_xx = grad_f_x[node][0];
            double hess_xy = 0.5 * (grad_f_x[node][1] + grad_f_y[node][0]);
            double hess_xz = 0.5 * (grad_f_x[node][2] + grad_f_z[node][0]);
            double hess_yy = grad_f_y[node][1];
            double hess_yz = 0.5 * (grad_f_y[node][2] + grad_f_z[node][1]);
            double hess_zz = grad_f_z[node][2];
            hess[node][0] = hess_xx;
            hess[node][1] = hess_xy;
            hess[node][2] = hess_xz;
            hess[node][3] = hess_yy;
            hess[node][4] = hess_yz;
            hess[node][5] = hess_zz;
        }

        Parfait::syncVector(mp, hess, node_g2l, node_sync_pattern);
        return hess;
    }

  private:
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> mesh;
    std::vector<std::vector<int>> node_stencils;
    std::vector<std::vector<Parfait::Point<double>>> node_stencil_coeffs;
    Parfait::SyncPattern node_sync_pattern;
    std::map<long, int> node_g2l;

    inline static double extractValue(int n, const inf::FieldInterface& f) {
        double d;
        f.value(n, &d);
        return d;
    }

    template <typename FieldAccessor>
    std::vector<double> extractHaloValues(const std::vector<int>& n2n, FieldAccessor f) const {
        std::vector<double> halo_values;
        for (size_t i = 0; i < n2n.size(); i++) {
            halo_values.push_back(f(n2n[i]));
        }
        return halo_values;
    }
};

class CellToNodeGradientCalculator {
  public:
    inline CellToNodeGradientCalculator(MessagePasser mp,
                                        std::shared_ptr<inf::MeshInterface> mesh,
                                        double distance_weight_power = 1.0)
        : mp(mp),
          mesh(mesh),
          node_to_cell(inf::NodeToCell::build(*mesh)),
          node_stencil_coeffs(mesh->nodeCount()),
          node_sync_pattern(inf::buildNodeSyncPattern(mp, *mesh)),
          node_g2l(inf::GlobalToLocal::buildNode(*mesh)) {
        node_stencils = node_to_cell;

        augmentStencils();

        auto cell_centroids = calcCellCentroids();

        auto calc_node_weights = [&](int n) {
            int num_neighbors = node_stencils[n].size();
            node_stencil_coeffs[n].resize(num_neighbors);
            if (mesh->nodeOwner(n) != mp.Rank()) return;
            auto get_neighbor_cell_centroid = [&](int neighbor) {
                int c = node_stencils[n][neighbor];
                auto p = cell_centroids[c];
                return p;
            };
            auto dimension =
                Parfait::detectStencilDimension(get_neighbor_cell_centroid, num_neighbors);
            Parfait::Point<double> node_location = mesh->node(n);
            if (num_neighbors >= 16) {
                auto row_coeffs =
                    Parfait::calcQLSQCoefficients(get_neighbor_cell_centroid,
                                                  num_neighbors,
                                                  distance_weight_power,
                                                  node_location,
                                                  Parfait::LSQDimensionality(dimension));

                for (int i = 0; i < num_neighbors; i++) {
                    node_stencil_coeffs[n][i][0] = row_coeffs(i, 1);
                    node_stencil_coeffs[n][i][1] = row_coeffs(i, 2);
                    if (dimension > 2)
                        node_stencil_coeffs[n][i][2] = row_coeffs(i, 3);
                    else
                        node_stencil_coeffs[n][i][2] = 0.0;
                }
            } else {
                auto row_coeffs =
                    Parfait::calcLLSQCoefficients(get_neighbor_cell_centroid,
                                                  num_neighbors,
                                                  distance_weight_power,
                                                  node_location,
                                                  Parfait::LSQDimensionality(dimension));

                for (int i = 0; i < num_neighbors; i++) {
                    node_stencil_coeffs[n][i][0] = row_coeffs(i, 1);
                    node_stencil_coeffs[n][i][1] = row_coeffs(i, 2);
                    if (dimension > 2)
                        node_stencil_coeffs[n][i][2] = row_coeffs(i, 3);
                    else
                        node_stencil_coeffs[n][i][2] = 0.0;
                }
            }
        };

        for (int n = 0; n < mesh->nodeCount(); n++) {
            calc_node_weights(n);
        }
    }

    inline void augmentStencils() {
        auto c2c = inf::CellToCell::build(*mesh);
        for (int n = 0; n < mesh->nodeCount(); n++) {
            std::set<int> stencil(node_stencils[n].begin(), node_stencils[n].end());
            for (auto c : node_stencils[n]) {
                for (int neighbor : c2c[c]) {
                    stencil.insert(neighbor);
                }
            }
            node_stencils[n] = std::vector<int>(stencil.begin(), stencil.end());
        }
    }

    inline std::vector<Parfait::Point<double>> calcGrad(const inf::FieldInterface& f) const {
        if (f.size() != mesh->cellCount() or f.association() != inf::FieldAttributes::Cell()) {
            PARFAIT_THROW(
                "Cannot compute cell to node gradient if field is not defined at cells with "
                "matching length");
        }
        auto f_accessor = [&f](int n) { return extractValue(n, f); };
        return calcGrad(f_accessor);
    }

    inline std::vector<std::array<double, 6>> calcHessian(const inf::FieldInterface& f) const {
        if (f.size() != mesh->cellCount() or f.association() != inf::FieldAttributes::Cell()) {
            PARFAIT_THROW(
                "Cannot compute cell to node gradient if field is not defined at cells with "
                "matching length");
        }
        auto f_accessor = [&f](int n) { return extractValue(n, f); };
        return calcHessian(f_accessor);
    }

    template <typename FieldAccessor>
    std::vector<Parfait::Point<double>> calcGrad(FieldAccessor f) const {
        std::vector<Parfait::Point<double>> grad(mesh->nodeCount(),
                                                 Parfait::Point<double>(0, 0, 0));

        for (int n = 0; n < mesh->nodeCount(); n++) {
            for (int i = 0; i < int(node_stencils[n].size()); i++) {
                for (int e = 0; e < 3; e++) {
                    grad[n][e] += node_stencil_coeffs[n][i][e] * f(node_stencils[n][i]);
                }
            }
        }
        Parfait::syncVector(mp, grad, node_g2l, node_sync_pattern);
        return grad;
    }

    template <typename FieldAccessor>
    std::vector<std::array<double, 6>> calcHessian(FieldAccessor f) const {
        auto grad = calcGrad(f);
        std::vector<std::array<double, 6>> hess(mesh->nodeCount());
        auto f_x_cell =
            inf::FieldTools::calcCellFromNodeAverage(*mesh, [&](int n) { return grad[n][0]; });
        auto f_y_cell =
            inf::FieldTools::calcCellFromNodeAverage(*mesh, [&](int n) { return grad[n][1]; });
        auto f_z_cell =
            inf::FieldTools::calcCellFromNodeAverage(*mesh, [&](int n) { return grad[n][2]; });

        auto f_x = [&](int c) { return f_x_cell[c]; };
        auto f_y = [&](int c) { return f_y_cell[c]; };
        auto f_z = [&](int c) { return f_z_cell[c]; };

        auto grad_f_x = calcGrad(f_x);
        auto grad_f_y = calcGrad(f_y);
        auto grad_f_z = calcGrad(f_z);

        for (int node = 0; node < mesh->nodeCount(); node++) {
            double hess_xx = grad_f_x[node][0];
            double hess_xy = 0.5 * (grad_f_x[node][1] + grad_f_y[node][0]);
            double hess_xz = 0.5 * (grad_f_x[node][2] + grad_f_z[node][0]);
            double hess_yy = grad_f_y[node][1];
            double hess_yz = 0.5 * (grad_f_y[node][2] + grad_f_z[node][1]);
            double hess_zz = grad_f_z[node][2];
            hess[node][0] = hess_xx;
            hess[node][1] = hess_xy;
            hess[node][2] = hess_xz;
            hess[node][3] = hess_yy;
            hess[node][4] = hess_yz;
            hess[node][5] = hess_zz;
        }

        Parfait::syncVector(mp, hess, node_g2l, node_sync_pattern);
        return hess;
    }

  private:
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> mesh;
    std::vector<std::vector<int>> node_to_cell;
    std::vector<std::vector<int>> node_stencils;
    std::vector<std::vector<Parfait::Point<double>>> node_stencil_coeffs;
    Parfait::SyncPattern node_sync_pattern;
    std::map<long, int> node_g2l;

    inline static double extractValue(int n, const inf::FieldInterface& f) {
        double d;
        f.value(n, &d);
        return d;
    }

    inline std::vector<Parfait::Point<double>> calcCellCentroids() const {
        std::vector<Parfait::Point<double>> cell_centers(mesh->cellCount());
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto cell = inf::Cell(*mesh, c);
            cell_centers[c] = cell.averageCenter();
        }
        return cell_centers;
    }
};

inline std::shared_ptr<FieldInterface> convertHessianToField(
    const std::vector<std::array<double, 6>>& hessian) {
    constexpr int entry_length = 6;
    auto field = std::make_shared<VectorFieldAdapter>(
        "hessian", FieldAttributes::Node(), entry_length, hessian.size());
    auto& v = field->getVector();
    for (size_t entry = 0; entry < hessian.size(); ++entry) {
        for (int i = 0; i < 6; ++i) {
            v[entry * entry_length + i] = hessian[entry][i];
        }
    }
    return field;
}
}
