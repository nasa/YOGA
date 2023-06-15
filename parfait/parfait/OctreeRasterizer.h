#pragma once

#include <queue>
#include <set>
#include <stack>
#include <vector>
#include "Extent.h"
#include "Octree.h"
#include "Throw.h"

namespace Parfait {
class OctreeRasterizer {
  public:
    OctreeRasterizer(Parfait::Extent<double> domain);

    void setMinSpacing(double spacing);

    template <typename T>
    void insert(const T& t) {
        insert(t, 0, 0);
    }

    template <typename T>
    void insert(const std::vector<T>& collection) {
        for (size_t i = 0; i < collection.size(); i++) {
            insert(collection[i], 0, 0);
            if (i % 100 == 0) {
                printf("Inserting %2.lf%%\r", 100 * (double)i / (double)collection.size());
            }
        }
        printf("Inserting Done!      \n");
        fflush(stdout);
    }

    inline std::vector<Parfait::Extent<double>> getFilledLeaves() {
        std::vector<Parfait::Extent<double>> out;
        for (size_t voxel_id = 0; voxel_id < extents.size(); voxel_id++) {
            if (isLeaf(voxel_id) and filled.at(voxel_id)) {
                out.push_back(extents[voxel_id]);
            }
        }
        return out;
    }

    inline void markStatus(const Parfait::Point<double>& outside_point) {
        std::vector<int> status(extents.size(), EMPTY);
        for (size_t i = 0; i < extents.size(); i++) {
            if (filled[i]) status[i] = CROSSING;
        }

        auto seed = leafAtPoint(outside_point, 0);

        floodFill(status, seed, OUTSIDE);

        for (size_t i = 0; i < extents.size(); i++) {
            if (status[i] == EMPTY) filled[i] = true;
        }

        refineFilled();
    }

  public:
    enum { EMPTY = -1, CROSSING = 0, INSIDE = 1, OUTSIDE = 2 };
    int max_depth = 8;
    Parfait::Extent<double> root_domain;
    std::vector<Parfait::Extent<double>> extents;
    std::vector<std::array<int, 8>> children;
    std::vector<bool> filled;

    template <typename T>
    void insert(const T& t, int voxel_id, int depth) {
        if (t.intersects(extents.at(voxel_id))) {
            filled.at(voxel_id) = true;
            if (not isLeaf(voxel_id)) {
                auto kids = children.at(voxel_id);
                for (auto& c : kids) {
                    if (c != EMPTY) insert(t, c, depth + 1);
                }
            } else {
                if (depth != max_depth) {
                    splitVoxel(voxel_id);
                    auto kids = children.at(voxel_id);
                    for (auto& c : kids) {
                        if (c != EMPTY) insert(t, c, depth + 1);
                    }
                }
            }
        }
    }

    inline int createVoxel(const Parfait::Extent<double>& e) {
        int id = extents.size();
        extents.push_back(e);
        std::array<int, 8> c;
        c.fill(-1);
        children.push_back(c);
        filled.push_back(false);
        return id;
    }

    inline bool isLeaf(int voxel_id) {
        auto& kids = children.at(voxel_id);
        for (auto& c : kids) {
            if (c != EMPTY) return false;
        }
        return true;
    }

    inline void splitVoxel(int voxel_id) {
        for (int i = 0; i < 8; i++) {
            auto cid = createVoxel(Octree::childExtent(extents.at(voxel_id), i));
            children.at(voxel_id).at(i) = cid;
        }
    }

    inline size_t leafAtPoint(const Parfait::Point<double>& p, size_t voxel_id) {
        if (isLeaf(voxel_id)) return voxel_id;
        for (auto& c : children[voxel_id]) {
            if (c != EMPTY) {
                if (extents[c].intersects(p)) return leafAtPoint(p, c);
            }
        }
        PARFAIT_THROW("Could not find leaf at point");
    }
    inline void floodFill(std::vector<int>& status, size_t seed, int mark) {
        std::stack<size_t> process;
        process.push(seed);
        double min_size = 0.5 / (1 << 21);

        while (not process.empty()) {
            auto v_id = process.top();
            process.pop();
            status[v_id] = mark;
            auto e = extents[v_id];
            e.lo -= Parfait::Point<double>(min_size, min_size, min_size);
            e.hi += Parfait::Point<double>(min_size, min_size, min_size);
            std::set<size_t> inside;
            getLeavesInsideExtent(0, e, inside);
            for (auto i : inside) {
                if (status[i] == EMPTY) process.push(i);
            }
        }
    }

    inline void getLeavesInsideExtent(int v_id, const Parfait::Extent<double>& e, std::set<size_t>& inside) {
        if (not extents[v_id].intersects(e)) return;

        if (isLeaf(v_id))
            inside.insert(v_id);
        else {
            for (auto& c : children[v_id]) {
                if (c != EMPTY) {
                    getLeavesInsideExtent(c, e, inside);
                }
            }
        }
    }
    void refineFilled();
};

inline OctreeRasterizer::OctreeRasterizer(Parfait::Extent<double> domain) {
    domain.makeIsotropic();
    root_domain = domain;
    createVoxel(root_domain);
    extents.reserve(1e7);
    children.reserve(1e7);
    filled.reserve(1e7);
}

inline void OctreeRasterizer::setMinSpacing(double spacing) {
    double length = root_domain.hi[0] - root_domain.lo[0];
    int depth = 0;
    while (true) {
        if (length <= spacing) break;
        length *= 0.5;
        depth++;
    }
    max_depth = depth;
    if (max_depth > 15) {
        PARFAIT_WARNING("max depth is " + std::to_string(max_depth));
        PARFAIT_WARNING("This is excessive.  But we're going to try anyway.");
        PARFAIT_WARNING("Hold onto something...");
    }
    printf("Inserting into max depth %d\n", max_depth);
}
inline void OctreeRasterizer::refineFilled() {
    size_t original_voxel_count = extents.size();
    for (size_t v_id = 0; v_id < original_voxel_count; v_id++) {
        if (isLeaf(v_id) and filled[v_id]) {
            auto e = extents[v_id];
            e.resize(0.9);
            insert(e);
        }
    }
}

}
