#pragma once
#include <parfait/Point.h>
#include <t-infinity/FieldInterface.h>

namespace inf {

class StructuredBlock {
  public:
    virtual ~StructuredBlock() = default;
    virtual std::array<int, 3> nodeDimensions() const = 0;
    std::array<int, 3> cellDimensions() const {
        auto dimensions = nodeDimensions();
        for (int& d : dimensions) d -= 1;
        return dimensions;
    }
    virtual Parfait::Point<double> point(int i, int j, int k) const = 0;
    virtual void setPoint(int i, int j, int k, const Parfait::Point<double>& p) = 0;
    virtual int blockId() const = 0;
};

class StructuredBlockField {
  public:
    virtual ~StructuredBlockField() = default;
    virtual std::array<int, 3> dimensions() const = 0;
    virtual double value(int i, int j, int k) const = 0;
    virtual void setValue(int i, int j, int k, double value) = 0;
    virtual int blockId() const = 0;
};

class StructuredMesh {
  public:
    virtual ~StructuredMesh() = default;
    virtual std::vector<int> blockIds() const = 0;
    virtual StructuredBlock& getBlock(int block_id) = 0;
    virtual const StructuredBlock& getBlock(int block_id) const = 0;
};

class StructuredField {
  public:
    virtual ~StructuredField() = default;
    virtual std::string name() const = 0;
    virtual std::string association() const = 0;
    virtual std::vector<int> blockIds() const = 0;
    virtual StructuredBlockField& getBlockField(int block_id) const = 0;
};
}