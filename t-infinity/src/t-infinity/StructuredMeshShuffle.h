#pragma once
#include <MessagePasser/MessagePasser.h>
#include "StructuredMesh.h"
#include "StructuredMeshGlobalIds.h"
#include <parfait/SyncPattern.h>

namespace inf {
std::shared_ptr<StructuredMesh> shuffleStructuredMesh(MessagePasser mp,
                                                      const StructuredMesh& mesh,
                                                      const std::map<int, int>& block_to_rank);
StructuredMeshGlobalIds shuffleStructuredMeshGlobalIds(MessagePasser mp,
                                                       const StructuredMeshGlobalIds& global,
                                                       const std::map<int, int>& block_to_rank);
Parfait::SyncPattern buildStructuredMeshtoMeshSyncPattern(const MessagePasser& mp,
                                                          const StructuredMeshGlobalIds& source,
                                                          const StructuredMeshGlobalIds& target);
}
