#pragma once
#include <array>
#include <functional>
#include <vector>
#include "Extent.h"
#include "Octree.h"
#include "ExtentBuilder.h"
#include "Throw.h"

namespace Parfait {
template <typename T>
class OctreeStorage {
  public:
    class Node {
      public:
        enum { EMPTY = -11 };
        inline Node(const Parfait::Extent<double>& e) : extent(e) {}
        int max_num_children = 8;
        Parfait::Extent<double> extent;
        std::array<int, 8> children{EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};

        std::vector<int> inside_objects;

        inline bool isLeaf() const {
            for (int i = 0; i < max_num_children; i++) {
                auto& c = children[i];
                if (c != EMPTY) return false;
            }
            return true;
        }
    };

  public:
    inline OctreeStorage() {
        objects.reserve(1e5);
        voxels.reserve(1e5);
        filled.reserve(1e5);
    }
    void setMaxDepth(int depth) { max_depth = depth; }
    void setRootExtent(const Parfait::Extent<double>& e) {
        root_extent = e;
        root_extent_set = true;
    }
    void setDimension(int d) {
        PARFAIT_ASSERT(d == 3 or d == 2, "Can only support 2 or 3 dimensions, not " + std::to_string(d));
        dimension = d;
        if (d == 2) {
            max_num_children = 4;
        }
    }
    void setMaxObjectsPerVoxel(int max) { max_objects_per_voxel = max; }
    void finalize() {
        if (locked) PARFAIT_THROW("Attempting to finalize a locked tree");
        if (voxels.size() == 0) {
            return;
        }
        if (filled.size() != voxels.size()) {
            PARFAIT_EXIT("Octree filled array size" + std::to_string(filled.size()) + " does not match voxels size " +
                         std::to_string(voxels.size()));
        }
        checkIndices(0);
        lazyDeleteEmptyChildren(0);
        contractExtents();
        locked = true;
        filled.clear();
        filled.shrink_to_fit();
    }
    void insert(const T* f) {
        if (not root_extent_set) PARFAIT_THROW("Cannot insert before setting root extent");
        if (locked) PARFAIT_THROW("Cannot insert after calling contractExtents()");
        if (voxels.size() == 0) initializeRoot();
        int object_id = objects.size();
        objects.push_back(f);
        insert(0, object_id, 0);
    }
    std::vector<std::pair<Parfait::Extent<double>, int>> exportLeafExtentsAndCounts() const {
        std::vector<std::pair<Parfait::Extent<double>, int>> output;
        for (auto& v : voxels) {
            if (v.isLeaf()) output.push_back({v.extent, v.inside_objects.size()});
        }
        return output;
    }
    std::vector<Parfait::Extent<double>> getLeafExtents() const {
        std::vector<Parfait::Extent<double>> extents;
        getLeafExtents(extents, 0);
        return extents;
    }

  public:
    int dimension = 3;
    int max_num_children = 8;
    int max_depth = 5;
    int max_objects_per_voxel = 10;
    int max_depth_achieved = 0;
    bool locked = false;
    bool root_extent_set = false;
    Parfait::Extent<double> root_extent;
    std::vector<Node> voxels;
    std::vector<bool> filled;
    std::vector<const T*> objects;

    inline void initializeRoot() {
        auto root = Node(root_extent);
        voxels.push_back(root);
        filled.push_back(false);
    }

    inline void splitVoxel(int voxel_index, int depth) {
        int before_voxel_count = voxels.size();
        for (int i = 0; i < max_num_children; i++) {
            voxels[voxel_index].children[i] = before_voxel_count + i;
            voxels.push_back(Node(Octree::childExtent(voxels[voxel_index].extent, i)));
            max_depth_achieved = std::max(max_depth_achieved, depth + 1);
            filled.push_back(false);
        }
        auto children = voxels[voxel_index].children;
        for (int i = 0; i < max_num_children; i++) {
            auto child = children[i];
            auto inside = voxels[voxel_index].inside_objects;
            for (auto& object_index : inside) {
                insert(child, object_index, depth + 1);
            }
        }
        voxels[voxel_index].inside_objects.clear();
        voxels[voxel_index].inside_objects.shrink_to_fit();
    }

    inline void insert(int voxel_index, int object_index, int depth) {
        const auto extent = voxels[voxel_index].extent;
        if (objects[object_index]->intersects(extent)) {
            filled[voxel_index] = true;
            if (voxels[voxel_index].isLeaf()) {
                voxels[voxel_index].inside_objects.push_back(object_index);
                if (depth != max_depth) {
                    if (int(voxels[voxel_index].inside_objects.size()) > max_objects_per_voxel) {
                        splitVoxel(voxel_index, depth);
                    }
                }
            } else {
                auto children = voxels[voxel_index].children;
                for (int i = 0; i < max_num_children; i++) {
                    auto child = children[i];
                    if (child != Node::EMPTY) insert(child, object_index, depth + 1);
                }
            }
        }
    }
    inline void contractExtents() {
        if (voxels.size() == 0) return;
        contractExtents(0);
    }
    inline Parfait::Extent<double> contractExtents(int voxel_index) {
        auto extent = determineShrunkExtent(voxel_index);
        voxels[voxel_index].extent = extent;
        return extent;
    }

    inline Extent<double> determineShrunkExtent(int voxel_index) {
        if (filled[voxel_index]) {
            if (voxels[voxel_index].isLeaf())
                return determineShrunkExtentLeaf(voxel_index);
            else
                return determineShrunkExtentChildren(voxel_index);
        }
        return voxels[voxel_index].extent;
    }

    inline Extent<double> determineShrunkExtentChildren(int voxel_index) {
        auto extent = ExtentBuilder::createEmptyBuildableExtent<double>();
        for (int i = 0; i < max_num_children; i++) {
            auto& child = voxels[voxel_index].children[i];
            if (child != Node::EMPTY) {
                if (filled[voxel_index]) ExtentBuilder::add(extent, contractExtents(child));
            }
        }
        return extent;
    }

    inline Extent<double> determineShrunkExtentLeaf(int voxel_index) const {
        auto extent = ExtentBuilder::createEmptyBuildableExtent<double>();
        for (long object_index : voxels[voxel_index].inside_objects) {
            const auto* object = objects[object_index];
            auto e = object->getExtent();
            auto overlap = ExtentBuilder::intersection(e, voxels[voxel_index].extent);
            ExtentBuilder::add(extent, overlap);
        }
        return extent;
    }

    inline void lazyDeleteEmptyChildren(int voxel_id) {
        for (int i = 0; i < max_num_children; i++) {
            auto& c = voxels[voxel_id].children[i];
            if (c != Node::EMPTY) {
                if (c < 0 or c >= long(filled.size())) {
                    PARFAIT_THROW("Voxel child " + std::to_string(c) + " out of bounds of filled array size " +
                                  std::to_string(filled.size()));
                }
                if (not filled[c])
                    c = Node::EMPTY;
                else
                    lazyDeleteEmptyChildren(c);
            }
        }
    }

    inline void getLeafExtents(std::vector<Parfait::Extent<double>>& extents, int voxel_id) const {
        if (voxels[voxel_id].isLeaf()) {
            extents.push_back(voxels[voxel_id].extent);
            return;
        } else {
            for (int i = 0; i < max_num_children; i++) {
                auto& c = voxels[voxel_id].children[i];
                if (c != Node::EMPTY) {
                    getLeafExtents(extents, c);
                }
            }
        }
    }
    inline void checkIndices(long voxel_id) {
        if (voxel_id < 0 or voxel_id >= long(voxels.size())) {
            PARFAIT_THROW("Trying to check voxel " + std::to_string(voxel_id) + " outside of array size " +
                          std::to_string(voxels.size()));
        }
        for (int i = 0; i < max_num_children; i++) {
            auto c = voxels.at(voxel_id).children[i];
            if (c != Node::EMPTY) {
                if (c < 0 or c > long(voxels.size())) {
                    PARFAIT_THROW("Voxel " + std::to_string(voxel_id) + " has child " + std::to_string(c) +
                                  " outside of array size " + std::to_string(voxels.size()));
                }
                checkIndices(c);
            }
        }
    }
};
}
