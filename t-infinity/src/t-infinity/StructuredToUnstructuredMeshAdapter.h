#pragma once
#include <unordered_map>
#include <MessagePasser/MessagePasser.h>
#include "StructuredMesh.h"
#include "MeshInterface.h"
#include "StructuredMeshGlobalIds.h"
#include "StructuredTinfMesh.h"

// Cells are wound according to CGNS HEXA_8 convention:
//
//   (1,1,1) 6  o------o 5 (1,0,1)
//             /|     /|
//  (0,1,0) 7 o------o |4 (0,0,1)          k   i
//   (1,1,0) 2| o----|-o 1 (1,0,0)         ^  ^
//            |/     |/                    | /
//  (0,1,0) 3 o------o 0 (0,0,0)    j <--- o
//
//  Face to block side mapping:
//
//                              F2
//    IMIN = F4              o------o
//    IMAX = F2             /| F5  /|
//    JMIN = F1            o------o |
//    JMAX = F3       F3   | o----|-o   F1
//    KMIN = F0            |/  F0 |/
//    KMAX = F5            o------o
//                           F4
//
//  Edge mapping:
//
//    E0  = ( 0,-1,-1)
//    E1  = (+1, 0,-1)                  o--E9--o
//    E2  = ( 0,+1,-1)        E10 ---> /E6    /|<--- E8
//    E3  = (-1, 0,-1)                o--E11-o E5
//    E4  = (-1,-1, 0)               E7 o-E1-|-o
//    E5  = (+1,-1, 0)        E2 ---> |/    E4/ <--- E0
//    E6  = (+1,+1, 0)                o--E3--o
//    E7  = (-1,+1, 0)
//    E8  = ( 0,-1,+1)
//    E9  = (+1, 0,+1)
//    E10 = ( 0,+1,+1)
//    E11 = (-1, 0,+1)
//

namespace inf {
constexpr std::array<int, 6> BlockSideToHexOrdering = {4, 2, 1, 3, 0, 5};
constexpr std::array<BlockSide, 6> HexOrderingToBlockSize = {KMIN, JMIN, IMAX, JMAX, IMIN, KMAX};
constexpr std::array<std::array<int, 3>, 8> HexNodeOrdering = {{
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 1, 1},
}};
constexpr std::array<std::array<int, 3>, 12> HexEdgeNeighborOffset = {{{0, -1, -1},
                                                                       {1, 0, -1},
                                                                       {0, 1, -1},
                                                                       {-1, 0, -1},
                                                                       {-1, -1, 0},
                                                                       {1, -1, 0},
                                                                       {1, 1, 0},
                                                                       {-1, 1, 0},
                                                                       {0, -1, 1},
                                                                       {1, 0, 1},
                                                                       {0, 1, 1},
                                                                       {-1, 0, 1}}};
constexpr std::array<std::array<int, 3>, 8> HexCornerNeighborOffset = {{{-1, -1, -1},
                                                                        {1, -1, -1},
                                                                        {1, 1, -1},
                                                                        {-1, 1, -1},
                                                                        {-1, -1, 1},
                                                                        {1, -1, 1},
                                                                        {1, 1, 1},
                                                                        {-1, 1, 1}}};
namespace StructuredMeshSurfaceCellTag {
    int encodeTag(int block_id, BlockSide side);
    int getBlockId(int tag);
    BlockSide getBlockSide(int tag);
}
std::shared_ptr<StructuredTinfMesh> convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp,
    const StructuredMesh& mesh,
    const StructuredMeshGlobalIds& global_nodes,
    const StructuredMeshGlobalIds& global_cells);
std::shared_ptr<StructuredTinfMesh> convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp, const StructuredMesh& mesh);
std::shared_ptr<StructuredTinfMesh> convertStructuredMeshToUnstructuredMesh(
    const MessagePasser& mp, const StructuredMesh& mesh, const MeshConnectivity& connectivity);
std::shared_ptr<FieldInterface> buildFieldInterfaceFromStructuredField(
    const StructuredField& structured_field);

StructuredBlockGlobalIds getGlobalCellIdsWithGhostCells(const MessagePasser& mp,
                                                        const StructuredMeshGlobalIds& global_cells,
                                                        const MeshInterface& mesh,
                                                        int num_ghost_cells_per_block_side);
StructuredBlockGlobalIds getGlobalNodeIdsWithGhostNodes(const MessagePasser& mp,
                                                        const StructuredMeshGlobalIds& global_cells,
                                                        const MeshInterface& mesh);
}
