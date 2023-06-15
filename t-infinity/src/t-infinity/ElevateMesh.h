#pragma once
#include <t-infinity/Cell.h>
#include <parfait/UniquePoints.h>
#include <parfait/PointWriter.h>
#include <parfait/ExtentBuilder.h>
#include "t-infinity/MeshHelpers.h"
#include "FieldInterface.h"
#include "Gradients.h"
namespace inf {

namespace P1ToP2 {
    inline auto getMapQuadratic() -> std::map<MeshInterface::CellType, MeshInterface::CellType> {
        using Type = inf::MeshInterface::CellType;
        std::map<MeshInterface::CellType, MeshInterface::CellType> m;
        m[Type::NODE] = Type::NODE;
        m[Type::BAR_2] = Type::BAR_3;
        m[Type::QUAD_4] = Type::QUAD_8;
        m[Type::TRI_3] = Type::TRI_6;
        m[Type::TETRA_4] = Type::TETRA_10;
        m[Type::PYRA_5] = Type::PYRA_13;
        m[Type::PENTA_6] = Type::PENTA_15;
        m[Type::HEXA_8] = Type::HEXA_20;
        return m;
    }
    inline auto getMapBiQuadratic() -> std::map<MeshInterface::CellType, MeshInterface::CellType> {
        using Type = inf::MeshInterface::CellType;
        std::map<MeshInterface::CellType, MeshInterface::CellType> m;
        m[Type::NODE] = Type::NODE;
        m[Type::BAR_2] = Type::BAR_3;
        m[Type::QUAD_4] = Type::QUAD_9;
        m[Type::TRI_3] = Type::TRI_6;
        m[Type::TETRA_4] = Type::TETRA_10;
        m[Type::PYRA_5] = Type::PYRA_14;
        m[Type::PENTA_6] = Type::PENTA_18;
        m[Type::HEXA_8] = Type::HEXA_27;
        return m;
    }
    inline auto getMap() { return getMapQuadratic(); }

    std::vector<Parfait::Point<double>> elevate(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType p1_type,
        MeshInterface::CellType inflated_type);
}

std::shared_ptr<inf::TinfMesh> elevateToP2(
    MessagePasser mp,
    const inf::TinfMesh& p1_mesh,
    std::map<MeshInterface::CellType, MeshInterface::CellType> type_map = inf::P1ToP2::getMap());

std::shared_ptr<inf::FieldInterface> cellToNodeReconstruction(
    const MeshInterface& mesh,
    const std::vector<Parfait::Point<double>>& cell_centroids,
    const std::vector<Parfait::Point<double>>& cell_gradients,
    const std::vector<std::vector<int>>& n2c,
    const FieldInterface& cell_field);

std::shared_ptr<inf::FieldInterface> interpolateCellFieldToNodes(
    const MeshInterface& p1_mesh,
    const MeshInterface& p2_mesh,
    const CellToNodeGradientCalculator& cell_to_node_grad_calculator,
    const std::vector<std::vector<int>>& node_to_cell_p2,
    const inf::FieldInterface& cell_field);

std::set<MeshInterface::CellType> extractAllCellTypes(MessagePasser mp, const TinfMesh& mesh);

}