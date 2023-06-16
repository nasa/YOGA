#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <memory>
#include <vector>

class InitialNodeSyncer {
  public:
    InitialNodeSyncer(MessagePasser mp);
    std::vector<std::vector<int>> createSendNodeMap(int num_ranks, const std::vector<int>& part) const;
    std::vector<long> syncOwnedGlobalNodes(const std::vector<long>& global_node_ids, const std::vector<int>& part);

  private:
    MessagePasser mp;
};