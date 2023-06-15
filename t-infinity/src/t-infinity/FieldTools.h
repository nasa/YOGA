#pragma once

#include <MessagePasser/MessagePasser.h>
#include <t-infinity/Cell.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/MeshInterface.h>

#include <memory>
#include <vector>
#include "VectorFieldAdapter.h"
#include "parfait/Checkpoint.h"
#include <parfait/Throw.h>
#include <parfait/StringTools.h>
#include <parfait/Dictionary.h>

namespace inf {
namespace FieldTools {

    std::vector<double> findMin(MessagePasser mp,
                                const inf::MeshInterface& mesh,
                                const FieldInterface& f);
    std::vector<double> findMax(MessagePasser mp,
                                const inf::MeshInterface& mesh,
                                const FieldInterface& f);
    double findNodalMin(MessagePasser mp,
                        const inf::MeshInterface& mesh,
                        const std::vector<double>& f);

    using FieldOperation = std::function<double(double, double)>;
    std::shared_ptr<FieldInterface> op(const std::string& name,
                                       const inf::FieldInterface& f1,
                                       const inf::FieldInterface& f2,
                                       FieldOperation op);
    std::shared_ptr<FieldInterface> diff(const inf::FieldInterface& f1,
                                         const inf::FieldInterface& f2);
    std::shared_ptr<FieldInterface> sum(const inf::FieldInterface& f1,
                                        const inf::FieldInterface& f2);
    std::shared_ptr<FieldInterface> multiply(const inf::FieldInterface& f1,
                                             const inf::FieldInterface& f2);
    std::shared_ptr<FieldInterface> divide(const inf::FieldInterface& f1,
                                           const inf::FieldInterface& f2);

    using ScalarFieldOperation = std::function<double(double)>;
    std::shared_ptr<FieldInterface> op(const std::string& name,
                                       const inf::FieldInterface& f,
                                       ScalarFieldOperation op);

    std::shared_ptr<FieldInterface> diff(const inf::FieldInterface& f1, double scalar);
    std::shared_ptr<FieldInterface> sum(const inf::FieldInterface& f1, double scalar);
    std::shared_ptr<FieldInterface> multiply(const inf::FieldInterface& f1, double scalar);
    std::shared_ptr<FieldInterface> divide(const inf::FieldInterface& f1, double scalar);
    std::shared_ptr<FieldInterface> abs(const inf::FieldInterface& f1);

    std::vector<double> getScalarFieldAsVector(const FieldInterface& f);
    std::shared_ptr<VectorFieldAdapter> createNodeFieldFromCallback(
        std::string name,
        const inf::MeshInterface& mesh,
        std::function<double(double, double, double)> callback);

    std::shared_ptr<FieldInterface> rename(const std::string& name, const inf::FieldInterface& f);

    template <typename T>
    void setSurfaceCellToVolumeNeighborValue(const inf::MeshInterface& mesh,
                                             const std::vector<std::vector<int>>& face_neighbors,
                                             int surface_cell_dimensionality,
                                             std::vector<T>& some_vector) {
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellDimensionality(c) == surface_cell_dimensionality) {
                int volume_neighbor = face_neighbors[c][0];
                PARFAIT_ASSERT(volume_neighbor >= 0 and volume_neighbor < mesh.cellCount(),
                               "Invalid volume neighbor");
                some_vector[c] = some_vector[volume_neighbor];
            }
        }
    }

    template <typename GetField>
    std::vector<double> calcNodeFromVolumeCellAverage(const inf::MeshInterface& mesh,
                                                      GetField getCellField,
                                                      const std::vector<std::vector<int>>& n2c) {
        std::vector<double> node_field(mesh.nodeCount(), 0.0);
        for (int node = 0; node < mesh.nodeCount(); node++) {
            auto neighbors = n2c[node];

            double total_volume = 0;
            for (auto neighbor : neighbors) {
                inf::Cell cell(mesh, neighbor);
                if (cell.dimension() != 3) continue;
                double area = cell.faceAreaNormal(0).magnitude();
                double length = sqrt(area);
                double volume = length * length * length;
                node_field[node] += getCellField(neighbor) * volume;
                total_volume += volume;
            }
            node_field[node] /= total_volume;
        }
        return node_field;
    }

    std::vector<double> calcNodeFromNeighborCellAverage(
        const inf::MeshInterface& mesh,
        std::function<double(int, int)> getCellField,
        const std::vector<std::vector<int>>& n2c);

    template <typename GetField>
    std::vector<double> calcNodeFromSurfaceCellAverage(const inf::MeshInterface& mesh,
                                                       GetField getCellField,
                                                       const std::vector<std::vector<int>>& n2c) {
        std::vector<double> node_field(mesh.nodeCount(), 0.0);
        for (int node = 0; node < mesh.nodeCount(); node++) {
            auto neighbors = n2c[node];

            double total_area = 0;
            for (auto neighbor : neighbors) {
                inf::Cell cell(mesh, neighbor);
                if (cell.dimension() != 2) continue;
                double area = cell.faceAreaNormal(0).magnitude();
                node_field[node] += getCellField(neighbor) * area;
                total_area += area;
            }
            node_field[node] /= total_area;
        }
        return node_field;
    }
    template <typename GetField>
    std::vector<double> calcCellFromNodeAverage(const inf::MeshInterface& mesh,
                                                GetField getNodeField) {
        std::vector<double> cell_field(mesh.cellCount(), 0.0);
        std::vector<int> nodes;
        nodes.reserve(8);
        for (int cell = 0; cell < mesh.cellCount(); cell++) {
            mesh.cell(cell, nodes);
            for (auto node : nodes) {
                cell_field[cell] += getNodeField(node);
            }
            cell_field[cell] /= double(nodes.size());
        }
        return cell_field;
    }
    std::shared_ptr<inf::FieldInterface> cellToNode_volume(
        const inf::MeshInterface& mesh,
        const inf::FieldInterface& f,
        const std::vector<std::vector<int>>& n2c);

    std::shared_ptr<inf::FieldInterface> cellToNode_volume(const inf::MeshInterface& mesh,
                                                           const inf::FieldInterface& f);

    std::shared_ptr<inf::VectorFieldAdapter> cellToNode_simpleAverage(
        const inf::MeshInterface& mesh,
        const inf::FieldInterface& f,
        const std::vector<std::vector<int>>& n2c);

    std::shared_ptr<inf::VectorFieldAdapter> cellToNode_simpleAverage(
        const inf::MeshInterface& mesh, const inf::FieldInterface& f);

    std::shared_ptr<inf::FieldInterface> cellToNode_surface(
        const inf::MeshInterface& mesh,
        const inf::FieldInterface& f,
        const std::vector<std::vector<int>>& n2c);

    std::shared_ptr<inf::FieldInterface> cellToNode_surface(const inf::MeshInterface& mesh,
                                                            const inf::FieldInterface& f);

    std::shared_ptr<inf::FieldInterface> nodeToCell(const inf::MeshInterface& mesh,
                                                    const inf::FieldInterface& f);
    std::shared_ptr<inf::FieldInterface> taubinSmoothing(
        const MessagePasser& mp,
        std::shared_ptr<MeshInterface> mesh,
        std::shared_ptr<FieldInterface> f,
        const std::vector<std::vector<int>>& stencil,
        Parfait::Dictionary options);

    std::shared_ptr<inf::FieldInterface> laplacianSmoothing(const inf::MeshInterface& mesh,
                                                            const inf::FieldInterface& f,
                                                            int passes = 20);
    std::shared_ptr<inf::FieldInterface> laplacianSmoothing(
        const inf::MeshInterface& mesh,
        const inf::FieldInterface& f,
        const std::vector<std::vector<int>>& stencil,
        int passes = 20);
    std::shared_ptr<FieldInterface> smoothField(const MessagePasser& mp,
                                                const std::string& type,
                                                std::shared_ptr<FieldInterface> field,
                                                std::shared_ptr<MeshInterface> mesh,
                                                const Parfait::Dictionary& options);

    template <typename T = double>
    std::vector<T> extractColumn(FieldInterface& input, int column) {
        std::vector<double> temp(input.blockSize());
        std::vector<T> output(input.size());
        for (int item = 0; item < input.size(); item++) {
            input.value(item, temp.data());
            output[item] = temp[column];
        }
        return output;
    }

    template <typename T = double>
    std::shared_ptr<inf::VectorFieldAdapter> extractColumnAsField(FieldInterface& input,
                                                                  int column) {
        auto output = extractColumn(input, column);
        return std::make_shared<inf::VectorFieldAdapter>(
            input.name(), input.association(), 1, output);
    }

    std::vector<std::shared_ptr<inf::FieldInterface>> split(
        std::shared_ptr<inf::FieldInterface> input);
    std::vector<std::shared_ptr<inf::FieldInterface>> split(
        std::shared_ptr<inf::FieldInterface> input, std::vector<std::string> names);
    std::shared_ptr<inf::FieldInterface> merge(
        std::vector<std::shared_ptr<inf::FieldInterface>> fields, std::string name);

    std::vector<double> gatherField(MessagePasser mp,
                                    const inf::MeshInterface& mesh,
                                    const inf::FieldInterface& field,
                                    int root);

    std::vector<double> gatherNodeField(MessagePasser mp,
                                        const inf::MeshInterface& mesh,
                                        const std::vector<double>& node_field,
                                        int root);

    std::vector<double> gatherCellField(MessagePasser mp,
                                        const inf::MeshInterface& mesh,
                                        const std::vector<double>& cell_field,
                                        int root);

    std::vector<double> gatherNodalScalarField(MessagePasser mp,
                                               const inf::MeshInterface& mesh,
                                               const inf::FieldInterface& field,
                                               int root);

    std::vector<double> gatherCellScalarField(MessagePasser mp,
                                              const inf::MeshInterface& mesh,
                                              const inf::FieldInterface& field,
                                              int root);

    std::vector<double> gatherScalarField(MessagePasser mp,
                                          const inf::MeshInterface& mesh,
                                          const inf::FieldInterface& field,
                                          int root);

    std::vector<double> fillAtNodes(const inf::MeshInterface& mesh,
                                    std::function<double(double, double, double)> function_of_xyz);

    std::shared_ptr<inf::FieldInterface> fillFieldAtNodes(
        const inf::MeshInterface& mesh,
        std::function<double(double, double, double)> function_of_xyz,
        std::string name = "field");

    double norm(MessagePasser mp,
                const std::vector<bool>& do_own,
                const inf::FieldInterface& f,
                double p);

    double spaldingYPlus(double uplus);
    double spaldingYPlusDerivative(double uplus);
    double calcSpaldingLawOfTheWallUPlus(double yplus);
    std::vector<double> calcSpaldingLawOfTheWallUPlus(double yplus,
                                                      const std::vector<double>& wall_distance);

}
}
