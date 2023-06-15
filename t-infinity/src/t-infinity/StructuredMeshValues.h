#pragma once

namespace inf {
template <typename T>
class StructuredMeshValues {
  public:
    virtual T getValue(int block_id, int i, int j, int k, int var) const = 0;
    virtual void setValue(int block_id, int i, int j, int k, int var, const T& p) = 0;
    virtual int blockSize() const = 0;
    virtual ~StructuredMeshValues() = default;
};
}