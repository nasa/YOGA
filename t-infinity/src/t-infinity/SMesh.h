#pragma once
#include <utility>
#include <memory>
#include <vector>
#include "MDVector.h"
#include <parfait/MapTools.h>
#include "StructuredMesh.h"

namespace inf {

class SMesh : public StructuredMesh {
  public:
    SMesh() = default;
    SMesh(const StructuredMesh& mesh) {
        for (int block_id : mesh.blockIds()) {
            blocks[block_id] = std::make_shared<Block>(mesh.getBlock(block_id));
        }
    }
    SMesh& operator=(const StructuredMesh& mesh) {
        blocks.clear();
        for (int block_id : mesh.blockIds())
            blocks[block_id] = std::make_shared<Block>(mesh.getBlock(block_id));
        return *this;
    }
    std::vector<int> blockIds() const override {
        auto ids = Parfait::MapTools::keys(blocks);
        return std::vector<int>(ids.begin(), ids.end());
    }
    const StructuredBlock& getBlock(int block_id) const override { return *blocks.at(block_id); }
    StructuredBlock& getBlock(int block_id) override { return *blocks[block_id]; }
    void add(const std::shared_ptr<StructuredBlock>& b) { blocks[b->blockId()] = b; }

    class Block : public StructuredBlock {
      public:
        explicit Block(const std::array<int, 3>& dimensions, int block_id_in = 0)
            : block_id(block_id_in), points(dimensions) {}
        Block(const StructuredBlock& b) : block_id(b.blockId()), points(copyPoints(b)) {}
        Block& operator=(const StructuredBlock& b) {
            block_id = b.blockId();
            points = copyPoints(b);
            return *this;
        }
        std::array<int, 3> nodeDimensions() const override { return points.dimensions(); }
        Parfait::Point<double> point(int i, int j, int k) const override { return points(i, j, k); }
        void setPoint(int i, int j, int k, const Parfait::Point<double>& p) override {
            points(i, j, k) = p;
        }
        int blockId() const override { return block_id; }
        using Points = Vector3D<Parfait::Point<double>>;

        static Points copyPoints(const StructuredBlock& block) {
            Points p(block.nodeDimensions());
            loopMDRange({0, 0, 0}, p.dimensions(), [&](int i, int j, int k) {
                p(i, j, k) = block.point(i, j, k);
            });
            return p;
        }

        int block_id;
        Points points;
    };

    static auto createBlock(const std::array<int, 3>& dimensions, int block_id = 0) {
        return std::make_shared<Block>(dimensions, block_id);
    }

    std::map<int, std::shared_ptr<StructuredBlock>> blocks;
};

class SField : public StructuredField {
  public:
    inline explicit SField(std::string name_in,
                           std::string association_in = inf::FieldAttributes::Unassigned())
        : _name(std::move(name_in)), _association(std::move(association_in)) {}

    inline std::string name() const override { return _name; }
    inline std::vector<int> blockIds() const override {
        auto ids = Parfait::MapTools::keys(field_blocks);
        return std::vector<int>(ids.begin(), ids.end());
    }
    inline StructuredBlockField& getBlockField(int block_id) const override {
        return *field_blocks.at(block_id);
    }
    inline void add(std::shared_ptr<StructuredBlockField> b) { field_blocks[b->blockId()] = b; }
    inline std::string association() const override { return _association; };

    class Block : public StructuredBlockField {
      public:
        explicit Block(const std::array<int, 3>& dimensions, int block_id = 0)
            : block_id(block_id), values(dimensions) {}
        std::array<int, 3> dimensions() const override { return values.dimensions(); }
        double value(int i, int j, int k) const override { return values(i, j, k); }
        void setValue(int i, int j, int k, double value) override { values(i, j, k) = value; }
        int blockId() const override { return block_id; }

        int block_id;
        Vector3D<double> values;
    };

    static auto createBlock(const std::array<int, 3>& dimensions, int block_id = 0) {
        return std::make_shared<Block>(dimensions, block_id);
    }

    std::string _name;
    std::map<int, std::shared_ptr<StructuredBlockField>> field_blocks;
    std::string _association;
};
}
