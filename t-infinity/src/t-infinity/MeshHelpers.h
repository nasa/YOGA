#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncPattern.h>
#include "MeshInterface.h"
#include <parfait/ToString.h>

namespace inf {

Parfait::SyncPattern buildNodeSyncPattern(MessagePasser mp, const MeshInterface& mesh);
Parfait::SyncPattern buildNodeSyncPattern(MessagePasser mp,
                                          const MeshInterface& mesh,
                                          const std::set<int>& subset_of_non_owned_nodes_to_sync);
Parfait::SyncPattern buildCellSyncPattern(MessagePasser mp, const MeshInterface& mesh);

long globalNodeCount(MessagePasser mp, const MeshInterface& mesh);
long globalCellCount(MessagePasser mp, const MeshInterface& mesh);
long globalCellCount(MessagePasser mp,
                     const MeshInterface& mesh,
                     MeshInterface::CellType cell_type);

bool is2D(MessagePasser mp, const MeshInterface& mesh);
bool isSimplex(const MessagePasser& mp, const inf::MeshInterface& mesh);
int maxCellDimensionality(MessagePasser mp, const MeshInterface& mesh);

std::vector<int> getCellOwners(const MeshInterface& mesh);
std::vector<long> getGlobalCellIds(const MeshInterface& mesh);
std::vector<long> getGlobalNodeIds(const MeshInterface& mesh);
std::vector<int> getNodeOwners(const MeshInterface& mesh);
}
