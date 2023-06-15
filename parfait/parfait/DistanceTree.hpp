
#pragma once
namespace Parfait {

inline DistanceTree::CurrentState::CurrentState(const Point<double> q)
    : query_point(q), actual_distance(std::numeric_limits<double>::max()), found_on_surface(false) {}

inline bool DistanceTree::CurrentState::isPotentiallyCloser(const Point<double> p) const {
    double relative_percent_bounds = 0.0;
    double d = (p - query_point).magnitude();
    return (d < (1.0 - relative_percent_bounds) * actual_distance);
}

inline void DistanceTree::CurrentState::changeLocationIfCloser(const Point<double> p, int new_index) {
    found_on_surface = true;
    double d = (p - query_point).magnitude();
    if (d < actual_distance) {
        actual_distance = d;
        surface_location = p;
        facet_index = new_index;
    }
}

inline double DistanceTree::CurrentState::actualDistance() const { return actual_distance; }

inline Point<double> DistanceTree::CurrentState::surfaceLocation() const { return surface_location; }

inline bool DistanceTree::CurrentState::found() const { return found_on_surface; }
inline int DistanceTree::CurrentState::facetIndex() const { return facet_index; }

inline DistanceTree::DistanceTree(Extent<double> e) {
    e.makeIsotropic();
    e.resize(1.001);
    setRootExtent(e);
}

inline void DistanceTree::printVoxelStatistics() {
    int total_count = 0;
    int leaf_count = 0;
    size_t max_facets = 0;
    long total_facets = 0;
    int count_too_big = 0;
    for (auto& v : voxels) {
        total_count++;
        if (v.isLeaf()) {
            max_facets = std::max(max_facets, v.inside_objects.size());
            total_facets += v.inside_objects.size();
            leaf_count++;
            if (v.inside_objects.size() > max_objects_per_voxel) count_too_big++;
        }
    }
    printf("There are %d voxels, %d leaves\n", total_count, leaf_count);
    printf("Storing %ld objects\n", objects.size());
    printf("Heaviest voxel has %lu facets.\n", max_facets);
    printf("Num facets over max = %d\n", count_too_big);
    printf("The average is %lf facets.\n", total_facets / float(leaf_count));
    printf("The deepest voxel is at depth %d\n", max_depth_achieved);
}

inline std::tuple<Parfait::Point<double>, int> DistanceTree::closestPointAndIndex(
    const Point<double>& p, const Parfait::Point<double>& known_surface_location) const {
    PriorityQueue process;
    process.push(std::make_pair(std::numeric_limits<double>::max(), 0));

    CurrentState current_state(p);
    current_state.changeLocationIfCloser(known_surface_location, -1);

    while (not process.empty()) {
        current_state = closestPoint(p, current_state, process);
        if (process.empty()) continue;
        if (current_state.actualDistance() < process.top().first) {
            break;
        }
    }
    if (not current_state.found()) PARFAIT_THROW("No surface location found for query point");
    return {current_state.surfaceLocation(), current_state.facetIndex()};
}

inline Point<double> DistanceTree::closestPoint(const Point<double>& p) const {
    Parfait::Point<double> o;
    o[0] = std::numeric_limits<double>::max();
    o[1] = std::numeric_limits<double>::max();
    o[2] = std::numeric_limits<double>::max();
    int out_index;
    std::tie(o, out_index) = closestPointAndIndex(p, o);
    return o;
}

inline Point<double> DistanceTree::closestPoint(const Point<double>& p,
                                                const Parfait::Point<double>& known_surface_location) const {
    Parfait::Point<double> o;
    int out_index;
    std::tie(o, out_index) = closestPointAndIndex(p, known_surface_location);
    return o;
}

inline std::tuple<Parfait::Point<double>, int> DistanceTree::closestPointAndIndex(const Point<double>& p) const {
    Parfait::Point<double> o;
    o[0] = std::numeric_limits<double>::max();
    o[1] = std::numeric_limits<double>::max();
    o[2] = std::numeric_limits<double>::max();
    return closestPointAndIndex(p, o);
}

inline DistanceTree::CurrentState DistanceTree::closestPoint(const Point<double>& query_point,
                                                             const DistanceTree::CurrentState& current_state,
                                                             DistanceTree::PriorityQueue& process) const {
    long voxel_index = process.top().second;
    process.pop();
    if (voxels.empty()) return current_state;

    auto closest_possible_to_voxel = voxels[voxel_index].extent.clamp(query_point);
    if (not current_state.isPotentiallyCloser(closest_possible_to_voxel)) {
        return current_state;
    }

    if (voxels[voxel_index].isLeaf()) {
        return getClosestPointInLeaf(voxel_index, query_point, current_state);
    }

    for (const auto& child : voxels[voxel_index].children) {
        if (child != Node::EMPTY) {
            const auto& child_extent = voxels[child].extent;
            auto p = child_extent.clamp(query_point);
            double dist = (p - query_point).magnitude();
            process.push(std::make_pair(dist, child));
        }
    }
    return current_state;
}

inline DistanceTree::CurrentState DistanceTree::getClosestPointInLeaf(int voxel_index,
                                                                      const Point<double>& query_point,
                                                                      DistanceTree::CurrentState current_state) const {
    for (const auto f : voxels[voxel_index].inside_objects) {
        auto p = objects[f]->getClosestPoint(query_point);
        current_state.changeLocationIfCloser(p, f);
    }
    return current_state;
}

inline FacetSegment::FacetSegment(const Facet& f) : Facet(f) {}

inline Extent<double> FacetSegment::getExtent() const { return Facet::getExtent(); }

inline Point<double> FacetSegment::getClosestPoint(const Point<double>& p) const { return Facet::getClosestPoint(p); }

inline bool FacetSegment::intersects(const Extent<double>& e) const { return Facet::intersects(e); }

inline TriP2Segment::TriP2Segment(const LagrangeTriangle::P2& tri) : LagrangeTriangle::P2(tri) {}

inline Extent<double> TriP2Segment::getExtent() const {
    auto e = ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto& p : points) {
        ExtentBuilder::add(e, p);
    }
    return e;
}

inline Point<double> TriP2Segment::getClosestPoint(const Point<double>& p) const {
    return LagrangeTriangle::P2::closest(p);
}

inline bool TriP2Segment::intersects(const Extent<double>& e) const {
    Facet f1(points[0], points[3], points[5]);
    Facet f2(points[3], points[1], points[4]);
    Facet f3(points[4], points[2], points[5]);
    Facet f4(points[3], points[4], points[5]);
    return f1.intersects(e) or f2.intersects(e) or f3.intersects(e) or f4.intersects(e);
}
}
