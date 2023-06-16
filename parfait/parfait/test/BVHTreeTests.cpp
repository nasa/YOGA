#include <RingAssertions.h>
#include <parfait/PointGenerator.h>
#include <parfait/PointWriter.h>
#include <parfait/RecursiveBisection.h>
#include <unordered_map>
#include <utility>
#include <parfait/ExtentWriter.h>
#include <parfait/Facet.h>
#include <parfait/STL.h>
#include <parfait/Timing.h>
#include <parfait/DistanceTree.h>

class BVHStorable {
  public:
    virtual Parfait::Point<double> getCenterPoint() const = 0;
    virtual Parfait::Extent<double> getExtent() const = 0;
    virtual ~BVHStorable() = default;
};

class PointStorable : public BVHStorable {
  public:
    PointStorable() = default;
    PointStorable(const Parfait::Point<double>& p) : p(p) {}
    Parfait::Point<double> getCenterPoint() const override { return p; }
    Parfait::Extent<double> getExtent() const override {
        Parfait::Point<double> eps{1.0e-9, 1.0e-9, 1.0e-9};
        return {p - eps, p + eps};
    }

  private:
    Parfait::Point<double> p;
};

class FacetStorable : public BVHStorable {
  public:
    FacetStorable() = default;
    FacetStorable(const Parfait::Facet& f) : f(f) {}
    Parfait::Point<double> getCenterPoint() const override {
        Parfait::Point<double> p = f[0];
        p += f[1];
        p += f[2];
        return 1.0 / 3.0 * p;
    }
    Parfait::Extent<double> getExtent() const override { return f.getExtent(); }

  private:
    Parfait::Facet f;
};

template <int BranchFactor>
class BVH {
  public:
    BVH(std::vector<BVHStorable*> items) : stored_items(std::move(items)) {
        voxel_children.reserve(1e5);
        voxel_parent_id.reserve(1e5);
        voxel_depth.reserve(1e5);
        int root = 0;
        voxel_children.resize(1, noChildren());
        voxel_parent_id.resize(1, -1);
        voxel_depth.resize(1, 1);

        auto all_points_in_root = std::vector<int>(stored_items.size());
        std::iota(all_points_in_root.begin(), all_points_in_root.end(), 0);

        pushItemsToVoxel(root, all_points_in_root);

        setExtents();
    }

    void setExtents() {
        voxel_extents.resize(currentNumberOfVoxels(), Parfait::ExtentBuilder::createEmptyBuildableExtent<double>());
        for (auto& key_value : items_in_leaves) {
            int leaf_voxel_id = key_value.first;
            auto& items = key_value.second;
            for (auto item : items) {
                Parfait::ExtentBuilder::add(voxel_extents[leaf_voxel_id], getItemExtent(item));
            }
            expandParentExtents(leaf_voxel_id);
        }
    }

    void expandParentExtents(int current_voxel_id) {
        int parent = voxel_parent_id[current_voxel_id];
        if (parent != -1) {
            Parfait::ExtentBuilder::add(voxel_extents[parent], voxel_extents[current_voxel_id]);
            expandParentExtents(parent);
        }
    }

  public:
    std::vector<BVHStorable*> stored_items;
    std::vector<std::array<int, BranchFactor>> voxel_children;
    std::vector<int> voxel_parent_id;
    std::vector<int> voxel_depth;
    std::unordered_map<int, std::vector<int>> items_in_leaves;

    std::vector<Parfait::Extent<double>> voxel_extents;
    int target_voxel_size = 20;

    std::array<int, BranchFactor> noChildren() {
        std::array<int, BranchFactor> out;
        out.fill(-1);
        return out;
    }

    Parfait::Extent<double> getItemExtent(int item_id) const { return stored_items[item_id]->getExtent(); }

    int currentNumberOfVoxels() const { return int(voxel_children.size()); }

    void createChildren(int parent_id) {
        int previous_voxel_count = currentNumberOfVoxels();
        voxel_children.resize(voxel_children.size() + BranchFactor, noChildren());
        voxel_parent_id.resize(voxel_children.size() + BranchFactor);
        voxel_depth.resize(voxel_children.size(), +BranchFactor);
        for (int i = 0; i < BranchFactor; i++) {
            int child_id = previous_voxel_count + i;
            voxel_children[parent_id][i] = child_id;
            voxel_parent_id[child_id] = parent_id;
            voxel_depth[child_id] = voxel_depth[parent_id] + 1;
        }
    }

    void pushItemsToVoxel(int current_voxel_id, const std::vector<int>& item_ids) {
        if (item_ids.size() < target_voxel_size) {
            items_in_leaves[current_voxel_id] = item_ids;
            return;
        }

        auto child_items = partitionItems(item_ids);
        int previous_voxel_count = currentNumberOfVoxels();
        createChildren(current_voxel_id);
        for (int i = 0; i < child_items.size(); i++) {
            int child_voxel_id = previous_voxel_count + i;
            auto& child_things = child_items[i];
            pushItemsToVoxel(child_voxel_id, child_things);
        }
    }

    std::vector<Parfait::Point<double>> getItemPoints(const std::vector<int>& item_ids) const {
        std::vector<Parfait::Point<double>> out(item_ids.size());
        for (size_t i = 0; i < item_ids.size(); i++) {
            out[i] = getItemPoint(item_ids[i]);
        }
        return out;
    }

    std::vector<std::vector<int>> partitionItems(const std::vector<int>& item_ids) {
        std::vector<std::vector<int>> output(BranchFactor);
        auto item_points = getItemPoints(item_ids);
        auto assigned_child_ids = Parfait::recursiveBisection(item_points, BranchFactor);

        for (size_t i = 0; i < assigned_child_ids.size(); i++) {
            output[assigned_child_ids[i]].push_back(item_ids[i]);
        }

        return output;
    }

    Parfait::Point<double> getItemPoint(int id) const { return stored_items[id]->getCenterPoint(); }

    int maxAchievedDepth() const {
        int max_depth = 0;
        for (auto d : voxel_depth) {
            max_depth = std::max(max_depth, d);
        }
        return max_depth;
    }

    std::vector<int> getVoxelsAtDepth(int depth) const {
        std::vector<int> vids;
        for (size_t vid = 0; vid < currentNumberOfVoxels(); vid++) {
            if (voxel_depth[vid] == depth) vids.push_back(vid);
        }
        return vids;
    }

    std::vector<Parfait::Extent<double>> getVoxelExtents(const std::vector<int>& voxel_ids) const {
        std::vector<Parfait::Extent<double>> out(voxel_ids.size());
        for (size_t i = 0; i < voxel_ids.size(); i++) {
            auto vid = voxel_ids[i];
            out[i] = voxel_extents[vid];
        }
        return out;
    }
};
#if 0

TEST_CASE("Oct tree comparison") {
    auto facets = Parfait::STL::load("planet_express.stl");
    std::vector<Parfait::FacetSegment> stored_facets(facets.size());
    for (size_t i = 0; i < stored_facets.size(); i++) {
        stored_facets[i] = Parfait::FacetSegment(facets[i]);
    }
    auto e = Parfait::STL::findDomain(facets);
    auto begin = Parfait::Now();
    auto begin_memory_mb = Tracer::usedMemoryMB();
    Parfait::DistanceTree oct(e);
    for (size_t i = 0; i < stored_facets.size(); i++) {
        oct.insert(&stored_facets[i]);
    }
    oct.finalize();
    auto end_memory_mb = Tracer::usedMemoryMB();
    auto end = Parfait::Now();
    printf("Time to construct Octree: %s, %ld, %ld MB\n",
           Parfait::readableElapsedTimeAsString(begin, end).c_str(),
           begin_memory_mb,
           end_memory_mb);
}

TEST_CASE("Build ADT comparison") {
    auto facets = Parfait::STL::load("planet_express.stl");
    auto e = Parfait::STL::findDomain(facets);

    auto begin = Parfait::Now();
    auto begin_memory_mb = Tracer::usedMemoryMB();
    Parfait::Adt3DExtent adt(e);
    adt.reserve(facets.size());
    for (int i = 0; i < int(facets.size()); i++) {
        adt.store(i, facets[i].getExtent());
    }
    auto end_memory_mb = Tracer::usedMemoryMB();
    auto end = Parfait::Now();
    printf("Time to construct ADT: %s, %ld, %ld MB\n",
           Parfait::readableElapsedTimeAsString(begin, end).c_str(),
           begin_memory_mb,
           end_memory_mb);
}

TEST_CASE("BVH Tree exists") {
    // auto points = Parfait::generateRandomPoints(500000);
    // Tracer::begin("bvh");
    // std::vector<PointStorable> stored_points(points.size());
    // for(size_t i =0; i < stored_points.size(); i++){
    //    stored_points[i] = PointStorable(points[i]);
    //}
    // std::vector<BVHStorable*> stored_point_ptrs(points.size());
    // for(size_t i =0; i < stored_points.size(); i++){
    //    stored_point_ptrs[i] = &stored_points[i];
    //}

    auto baseline_memory = Tracer::usedMemoryMB();
    printf("BVH baseline memory %ld\n", baseline_memory);
    auto facets = Parfait::STL::load("planet_express.stl");
    std::vector<FacetStorable> stored_facets(facets.size());
    for (size_t i = 0; i < stored_facets.size(); i++) {
        stored_facets[i] = FacetStorable(facets[i]);
    }
    std::vector<BVHStorable*> stored_facet_ptrs(facets.size());
    for (size_t i = 0; i < stored_facets.size(); i++) {
        stored_facet_ptrs[i] = &stored_facets[i];
    }

    Tracer::setProcessId(0);

    auto begin = Parfait::Now();
    auto begin_memory_mb = Tracer::usedMemoryMB();
    BVH<8> bvh(stored_facet_ptrs);
    auto end = Parfait::Now();
    auto end_memory_mb = Tracer::usedMemoryMB();
    printf("Time to construct BVH: %s, %ld, %ld MB\n",
           Parfait::readableElapsedTimeAsString(begin, end).c_str(),
           begin_memory_mb,
           end_memory_mb);
    Tracer::end("bvh");

    for (int d = 0; d < bvh.maxAchievedDepth(); d++) {
        auto extents = bvh.getVoxelExtents(bvh.getVoxelsAtDepth(d));
        Parfait::ExtentWriter::write("bvh-depth" + std::to_string(d), extents);
    }

    printf("Max depth achieved %d\n", bvh.maxAchievedDepth());
}

#endif