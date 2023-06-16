#pragma once
#include <MessagePasser/MessagePasser.h>
#include <memory>
#include <algorithm>
#include <queue>
#include <t-infinity/MeshBuilder.h>
#include <parfait/PointGenerator.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>
#include <parfait/Linspace.h>
#include <t-infinity/Shortcuts.h>
#include "t-infinity/MeshHelpers.h"
#include "parfait/PointWriter.h"
#include "parfait/VectorTools.h"
#include "parfait/Checkpoint.h"
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/Geometry.h>
#include <t-infinity/MeshQualityMetrics.h>
namespace inf {

class Mechanics {
  public:
    Mechanics(MessagePasser mp, const std::string& viz_name) : mp(mp), viz_name(viz_name) {}

    double calcCavityCost(const std::vector<inf::experimental::CellTypeAndNodeIds>& cells_in_cavity,
                          const inf::TinfMesh& mesh);

    int getIdOfBestSteinerNodeForCavity(inf::experimental::MikeCavity& cavity,
                                        inf::experimental::MeshBuilder& builder);

    std::pair<double, double> calcMinAndMaxEdgeLength(
        const inf::TinfMesh& mesh,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        int dimension);

    bool edgesMatch(const std::array<int, 2>& a, const std::array<int, 2>& b);

    std::vector<inf::experimental::CellEntry> getCellsContainingEdge(const inf::TinfMesh& mesh,
                                                                     const std::array<int, 2>& edge,
                                                                     int dimension);
    bool areAnyOfTheseCellsOnTheBoundary(
        int dimension, const std::vector<inf::experimental::CellEntry>& cell_list);
    bool isEdgeOnBoundary(const inf::TinfMesh& mesh,
                          const std::array<int, 2>& edge,
                          int dimension) {
        return areAnyOfTheseCellsOnTheBoundary(dimension,
                                               getCellsContainingEdge(mesh, edge, dimension));
    }
    bool isCellSameDimensionOrOneLess(const inf::TinfMesh& mesh,
                                      int dimension,
                                      inf::MeshInterface::CellType& cell_type) {
        return dimension == mesh.cellTypeDimensionality(cell_type) or
               dimension == mesh.cellTypeDimensionality(cell_type) + 1;
    }

    void copyBuilderAndSyncAndVisualize(
        inf::experimental::MeshBuilder builder_copy,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length);

    std::vector<inf::experimental::CellEntry> identifyTopPercentOfPoorQualityCells(
        const inf::experimental::MeshBuilder& builder, double percent);

    std::vector<inf::experimental::CellEntry> identifyCellsWithLongestOrShortestEdgesInMetric(
        const inf::TinfMesh& mesh,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent,
        int dimension,
        bool invert);

    std::vector<std::array<int, 2>> getLongestEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent);

    std::vector<std::array<int, 2>> getShortestEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent);

    std::vector<std::array<int, 2>> sortEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent,
        bool long_to_short);

    std::vector<inf::experimental::CellEntry> identifyCellsWithLongestEdgesInMetric(
        const inf::TinfMesh& mesh,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent,
        int dimension) {
        return identifyCellsWithLongestOrShortestEdgesInMetric(
            mesh, calc_edge_length, percent, dimension, false);
    }

    std::vector<inf::experimental::CellEntry> identifyCellsWithShortestEdgesInMetric(
        const inf::TinfMesh& mesh,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double percent,
        int dimension) {
        return identifyCellsWithLongestOrShortestEdgesInMetric(
            mesh, calc_edge_length, percent, dimension, true);
    }

    int subdivideCell(inf::experimental::MeshBuilder& builder,
                      inf::experimental::CellEntry cell_entry,
                      Parfait::Point<double> p);

    bool isNodeCriticalOnGeometry(inf::experimental::MeshBuilder& builder, int node_id) {
        for (auto& nbr : builder.node_to_cells[node_id])
            if (inf::MeshInterface::NODE == nbr.type) return true;
        return false;
    }

    bool isEitherNodeOnBoundary(inf::experimental::MeshBuilder& builder, std::array<int, 2> edge) {
        auto& nbrs = builder.node_to_cells[edge[0]];
        std::vector<inf::experimental::CellEntry> cells_to_check(nbrs.begin(), nbrs.end());
        if (areAnyOfTheseCellsOnTheBoundary(builder.dimension, cells_to_check)) return true;
        auto& right_nbrs = builder.node_to_cells[edge[1]];
        std::vector<inf::experimental::CellEntry> cells_to_check_right(right_nbrs.begin(),
                                                                       right_nbrs.end());
        if (areAnyOfTheseCellsOnTheBoundary(builder.dimension, cells_to_check_right)) return true;
        return false;
    }

    bool collapseEdge(
        inf::experimental::MeshBuilder& builder,
        std::array<int, 2> edge,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length);

    void splitEdge(
        inf::experimental::MeshBuilder& builder,
        std::array<int, 2> edge,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length);

    bool swapEdge(inf::experimental::MeshBuilder& builder, std::array<int, 2> edge);

    int splitAllEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double edge_split_threshold);

    void collapseAllEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
        double edge_collapse_threshold);

    void swapAllEdges(
        inf::experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length);

    int insertNode(experimental::MeshBuilder& builder, Parfait::Point<double> p);

    bool combineNeighborsToQuad(experimental::MeshBuilder& builder,
                                std::array<int, 2> edge,
                                double cost_threshold = 0.3);

    void tryMakingQuads(
        experimental::MeshBuilder& builder,
        std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length);

  private:
    MessagePasser mp;
    const std::string viz_name;
    int viz_step = 0;
    std::vector<inf::experimental::CellEntry> getCellsOverlappingNode(
        int node_id, experimental::MeshBuilder& builder);
};
}