#pragma once

#include <MessagePasser/MessagePasser.h>
#include <vector>
#include "NaiveMesh.h"

namespace NC {
class Partitioner {
  public:
    virtual std::vector<int> generatePartVector(MessagePasser mp, const NaiveMesh& nm) const = 0;
    static std::unique_ptr<Partitioner> getPartitioner(MessagePasser mp, std::string partitioner_name = "default");
    virtual ~Partitioner() = default;
};
}