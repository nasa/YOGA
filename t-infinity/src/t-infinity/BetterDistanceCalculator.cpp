#include "BetterDistanceCalculator.h"

void printMaxMemory(MessagePasser mp, std::string message) {
    auto mem = Tracer::usedMemoryMB();
    auto max = mp.ParallelMax(mem);
    mp_rootprint("%s. Root: %ld MB, Max in Parallel: %ld\n", message.c_str(), mem, max);
}

Parfait::Point<double> inf::furthestPossiblePoint() {
    // Create a point that can safely have it's magnitude taken
    // without causing floating point exceptions
    double far = std::numeric_limits<double>::max();
    far = sqrt(far);
    far *= 0.25;
    Parfait::Point<double> maximum_point = {far, far, far};
    return maximum_point;
}
std::tuple<std::vector<double>, std::vector<double>> inf::calcBetterDistance(
    MessagePasser mp,
    const std::vector<Parfait::Facet>& facets,
    const std::vector<Parfait::Point<double>>& points,
    int number_of_trees,
    int max_depth,
    int max_facets_per_voxel) {
    std::vector<double> dist, search;
    std::vector<int> mock_metadata(facets.size(), 0);

    std::tie(dist, search, mock_metadata) = calcBetterDistanceWithMetaData(
        mp, facets, points, mock_metadata, number_of_trees, max_depth, max_facets_per_voxel);
    return {dist, search};
}
std::tuple<std::vector<double>, std::vector<double>, std::vector<int>> inf::calcBetterDistance(
    MessagePasser mp,
    const inf::MeshInterface& mesh,
    std::set<int> tags,
    const std::vector<Parfait::Point<double>>& points,
    int number_of_trees,
    int max_depth,
    int max_facets_per_voxel) {
    tags = mp.ParallelUnion(tags);
    std::vector<Parfait::Facet> facets;
    std::vector<int> facet_tags;
    std::tie(facets, facet_tags) = inf::extractOwned2DFacetsAndTheirTags(mp, mesh, tags);
    std::vector<double> distance, search_cost;
    std::tie(distance, search_cost, facet_tags) = calcBetterDistanceWithMetaData(
        mp, facets, points, facet_tags, number_of_trees, max_depth, max_facets_per_voxel);
    if (distance.size() != facet_tags.size()) {
        PARFAIT_THROW("We should have a tag for each query point.");
    }
    return {distance, search_cost, facet_tags};
}
std::tuple<std::vector<double>, std::vector<double>, std::vector<int>> inf::calcDistanceToNodes(
    MessagePasser mp,
    const inf::MeshInterface& mesh,
    const std::set<int>& tags,
    int number_of_trees,
    int max_depth,
    int max_facets_per_voxel) {
    return calcBetterDistance(
        mp, mesh, tags, extractPoints(mesh), number_of_trees, max_depth, max_facets_per_voxel);
}
inline double inf::closestDistanceBetween(const Parfait::Extent<double>& e1,
                                          const Parfait::Extent<double>& e2) {
    return (e1.center() - e2.center()).magnitude() - e1.radius() - e2.radius();
}
inline double inf::furthestDistanceBetween(const Parfait::Extent<double>& e1,
                                           const Parfait::Extent<double>& e2) {
    return (e1.center() - e2.center()).magnitude() + e1.radius() + e2.radius();
}
void inf::dist::traceMemoryParallel(MessagePasser mp) {
    long u = Tracer::usedMemoryMB();
    long a = Tracer::availableMemoryMB();
    auto parallel_max_used = mp.ParallelMax(u);
    auto parallel_min_avail = mp.ParallelMin(a);
    auto parallel_avg_used = mp.ParallelSum(u) / mp.NumberOfProcesses();
    auto max_rank = mp.ParallelRankOfMax(u);

    Tracer::counter("MPI Max Rank Used Memory (MB)", parallel_max_used);
    Tracer::counter("MPI Max Rank ", max_rank);
    Tracer::counter("MPI Avg Rank Used Memory (MB)", parallel_avg_used);
    Tracer::counter("MPI Min Node Remaining Memory (MB)", parallel_min_avail);
    long balance = 100 * (double)parallel_avg_used / (double)parallel_max_used;
    Tracer::counter("MPI Mem Load Balance (avg / max)", balance);
    Tracer::traceMemory();
}
