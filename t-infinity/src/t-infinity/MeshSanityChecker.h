#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Metrics.h>
#include <parfait/Throw.h>
#include <t-infinity/MeshInterface.h>

#include <array>
#include <set>
#include <Tracer.h>

#include "Cell.h"
#include "FaceNeighbors.h"
#include "GlobalToLocal.h"
#include "Hiro.h"

namespace inf {
struct HangingEdgeNeighborhood {
    int quad_cell = -1;
    int tri_cell_1 = -1;
    int tri_cell_2 = -1;

    inline void setQuadCell(int c) {
        if (quad_cell < 0) quad_cell = c;
    }

    inline void addTriCell(int c) {
        if (tri_cell_1 < 0) {
            tri_cell_1 = c;
            return;
        }
        if (tri_cell_2 < 0) {
            tri_cell_2 = c;
            return;
        }
    }

    inline bool isHangingEdge() { return (quad_cell >= 0 and tri_cell_1 >= 0 and tri_cell_2 >= 0); }
};
class MeshSanityChecker {
  public:
    typedef std::map<std::string, bool> ResultLog;

    static bool isMeshValid(const MeshInterface& mesh, MessagePasser mp);
    static ResultLog checkAll(const MeshInterface& mesh, MessagePasser mp);
    static ResultLog checkAllSerial(MessagePasser mp, const MeshInterface& mesh);
    static bool areCellFacesValid(const Cell& cell);
    static std::vector<double> minimumCellCentroidDistanceInNodeStencil(
        const MeshInterface& mesh, const std::vector<std::vector<int>>& n2c);

    static std::vector<double> minimumCellHiroCentroidDistanceInNodeStencil(
        const MeshInterface& mesh, const std::vector<std::vector<int>>& n2c, double hiro_p);
    static HangingEdgeNeighborhood checkForTwoTriFacesOnQuad(
        const MeshInterface& mesh, int cell_id, const std::vector<std::vector<int>>& n2c);
    static bool checkGlobalNodeIds(const MeshInterface& mesh);
    static bool checkCellOwners(const MeshInterface& mesh);
    static bool checkDuplicatedFaceNeighbors(const MeshInterface& mesh,
                                             const std::vector<std::vector<int>>& n2c,
                                             const std::vector<std::vector<int>>& face_neighbors);
    static bool checkGlobalCellIds(const MeshInterface& mesh);
    static bool isCellClosed(const inf::Cell& cell);
    static bool isQuadAllignedConsistently(const std::vector<Parfait::Point<double>>& face_points);
    static bool checkVolumeCellsAreClosed(const inf::MeshInterface& mesh);
    static bool checkFaceWindings(const inf::MeshInterface& mesh);
    static bool checkPositiveVolumes(const inf::MeshInterface& mesh);
    static bool checkOwnedCellsHaveFaceNeighbors(
        const MeshInterface& mesh,
        const std::vector<std::vector<int>>& n2c,
        const std::vector<std::vector<int>>& face_neighbors);
    static bool checkForSliverCells(MessagePasser mp,
                                    const MeshInterface& mesh,
                                    const std::vector<std::vector<int>>& c2c_maybe_face_neighbors,
                                    double sliver_cell_threshold = 1.0e-4);
    static std::vector<HangingEdgeNeighborhood> findHangingEdges(const MeshInterface& mesh);
    static std::vector<HangingEdgeNeighborhood> findHangingEdges(
        const MeshInterface& mesh,
        const std::vector<std::vector<int>>& n2c,
        std::vector<std::vector<int>>& face_neighbors);
};
}
