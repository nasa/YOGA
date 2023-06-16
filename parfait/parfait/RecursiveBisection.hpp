#include "RecursiveBisection.h"
#include <parfait/ExtentBuilder.h>
#include <parfait/Linspace.h>
#include <parfait/PointWriter.h>
#include <parfait/PointGenerator.h>
#include <parfait/ParallelExtent.h>
#include <thread>
#include "Throw.h"

namespace Parfait {
inline double calcCutRatio(int num_partitions) {
    int half = num_partitions / 2;
    return double(half) / double(num_partitions);
}
inline int numberOfLeftPartitions(int num_partitions) { return num_partitions / 2; }
inline int numberOfRightPartitions(int num_partitions) {
    return num_partitions - numberOfLeftPartitions(num_partitions);
}

template <size_t N>
size_t countLessThan(const std::vector<Parfait::Point<double, N>>& points, double center, int dimension) {
    size_t count = 0;
    for (auto& p : points) {
        if (p[dimension] < center) {
            count++;
        }
    }
    return count;
}

template <size_t N>
double sumLessThan(const std::vector<Parfait::Point<double, N>>& points,
                   const std::vector<double>& weights,
                   double center,
                   int dimension) {
    if (points.size() != weights.size()) {
        PARFAIT_THROW("weights incompatible!");
    }
    double sum = 0;
    for (size_t i = 0; i < points.size(); i++) {
        auto& p = points[i];
        if (p[dimension] < center) {
            sum += weights[i];
        }
    }
    return sum;
}

inline bool isWithinTol(double target_ratio, size_t left_count, size_t right_count, double tol) {
    double actual_ratio = static_cast<double>(left_count) / static_cast<double>(left_count + right_count);
    double error = std::fabs(actual_ratio - target_ratio) / target_ratio;
    double safety_ratio = 10.0 / (left_count + right_count);
    if (error < tol + safety_ratio) return true;
    return false;
}

template <typename T, size_t N>
inline void resizeDomainAndShiftCenter(
    Parfait::Extent<double, N>& domain, double& center, T num_left, T target_num_left, int dimension) {
    if (num_left < target_num_left) {
        domain.lo[dimension] = center;
    } else {
        domain.hi[dimension] = center;
    }
    center = 0.5 * (domain.lo[dimension] + domain.hi[dimension]);
}
template <size_t N>
double findCenter(Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol,
                  int iteration) {
    if (iteration > 100) return center;
    size_t left_count = countLessThan(points, center, dimension);
    size_t total_points = points.size();
    size_t right_count = total_points - left_count;
    size_t target_left_count = total_points * target_left_percentage;
    if (isWithinTol(target_left_percentage, left_count, right_count, tol)) {
        return center;
    } else {
        resizeDomainAndShiftCenter(domain, center, left_count, target_left_count, dimension);
        return findCenter(domain, points, target_left_percentage, center, dimension, tol, iteration + 1);
    }
}

inline void normalizeWeights(std::vector<double>& weights) {
    double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    double factor = double(weights.size()) / total;
    for (auto& w : weights) w *= factor;
}
template <size_t N>
double findCenter(Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  const std::vector<double>& weights,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol,
                  int iteration) {
    if (iteration > 100) return center;
    double total_sum = std::accumulate(weights.begin(), weights.end(), 0.0);
    double left_sum = sumLessThan(points, weights, center, dimension);

    double left_percentage = left_sum / total_sum;
    if (std::fabs(left_percentage - target_left_percentage) < tol) {
        return center;
    } else {
        resizeDomainAndShiftCenter(domain, center, left_percentage, target_left_percentage, dimension);
        return findCenter(domain, points, weights, target_left_percentage, center, dimension, tol, iteration + 1);
    }
}
template <size_t N>
double findCenter(MessagePasser mp,
                  Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol,
                  int iteration) {
    if (iteration > 100) return center;
    size_t left_count = countLessThan(points, center, dimension);
    left_count = mp.ParallelSum(left_count);
    size_t total_points = mp.ParallelSum(points.size());
    size_t right_count = total_points - left_count;
    size_t target_left_count = total_points * target_left_percentage;

    if (isWithinTol(target_left_percentage, left_count, right_count, tol)) {
        return center;
    } else {
        resizeDomainAndShiftCenter(domain, center, left_count, target_left_count, dimension);
        return findCenter(mp, domain, points, target_left_percentage, center, dimension, tol, iteration + 1);
    }
}

template <size_t N>
double findCenter(MessagePasser mp,
                  Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  const std::vector<double>& weights,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol,
                  int iteration) {
    if (iteration > 100) return center;
    double total_sum = std::accumulate(weights.begin(), weights.end(), 0.0);
    total_sum = mp.ParallelSum(total_sum);
    double left_sum = sumLessThan(points, weights, center, dimension);
    left_sum = mp.ParallelSum(left_sum);
    double left_percentage = left_sum / total_sum;

    if (std::fabs(left_percentage - target_left_percentage) < tol) {
        return center;
    } else {
        resizeDomainAndShiftCenter(domain, center, left_percentage, target_left_percentage, dimension);
        return findCenter(mp, domain, points, weights, target_left_percentage, center, dimension, tol, iteration + 1);
    }
}
template <size_t N>
std::tuple<std::vector<int>, std::vector<int>> splitAlongDimensionAt(
    const std::vector<Parfait::Point<double, N>>& points, int dimension, double c) {
    std::vector<int> points_left, points_right;
    for (size_t i = 0; i < points.size(); i++) {
        if (points[i][dimension] < c)
            points_left.push_back(i);
        else
            points_right.push_back(i);
    }
    return {points_left, points_right};
}

template <typename T>
std::vector<T> extract(const std::vector<T>& points, const std::vector<int>& ids) {
    std::vector<T> out;
    for (size_t id : ids) {
        out.push_back(points[id]);
    }
    return out;
}

template <size_t N>
std::vector<int> recursiveBisection_serial(std::vector<Parfait::Point<double, N>> points,
                                           const std::vector<double>& weights,
                                           unsigned int leaf_id,
                                           unsigned int depth,
                                           int num_partitions,
                                           double tol) {
    if (num_partitions == 0) {
        throw std::logic_error("RecursiveBisection: asking for 0 partitions doesn't make sense");
    }
    if (num_partitions == 1 or points.size() == 0) {
        return std::vector<int>(points.size(), leaf_id);
    }

    Parfait::wiggle(points);

    auto domain = Parfait::ExtentBuilder::build<decltype(points), N>(points);
    int dimension = domain.longestDimension();
    auto center = domain.center()[dimension];
    double ratio = calcCutRatio(num_partitions);
    double c = Parfait::findCenter(domain, points, weights, ratio, center, dimension, tol);
    std::vector<int> indices_left, indices_right;
    std::tie(indices_left, indices_right) = Parfait::splitAlongDimensionAt(points, dimension, c);
    auto points_left = Parfait::extract(points, indices_left);
    auto weights_left = Parfait::extract(weights, indices_left);
    auto points_right = Parfait::extract(points, indices_right);
    auto weights_right = Parfait::extract(weights, indices_right);
    unsigned int left_leaf_id = pow(2, depth) + leaf_id * 2;
    unsigned int right_leaf_id = pow(2, depth) + (leaf_id * 2) + 1;
    auto partition_ids_left = recursiveBisection_serial(
        points_left, weights_left, left_leaf_id, depth + 1, numberOfLeftPartitions(num_partitions), tol);
    auto partition_ids_right = recursiveBisection_serial(
        points_right, weights_right, right_leaf_id, depth + 1, numberOfRightPartitions(num_partitions), tol);

    std::vector<int> partition_ids(points.size());
    for (size_t i = 0; i < partition_ids_left.size(); i++) {
        int index = indices_left[i];
        partition_ids[index] = partition_ids_left[i];
    }
    for (size_t i = 0; i < partition_ids_right.size(); i++) {
        int index = indices_right[i];
        partition_ids[index] = partition_ids_right[i];
    }
    return partition_ids;
}

template <size_t N>
std::vector<int> recursiveBisection(std::vector<Parfait::Point<double, N>> points,
                                    const std::vector<double>& weights,
                                    int num_partitions,
                                    double tol) {
    auto part_ids = recursiveBisection_serial(points, weights, 0, 0, num_partitions, tol);
    std::set<int> unique_ids(part_ids.begin(), part_ids.end());
    std::map<int, int> old_to_new;
    int next_new = 0;
    for (auto old : unique_ids) old_to_new[old] = next_new++;

    for (auto& p : part_ids) p = old_to_new.at(p);

    return part_ids;
}
template <size_t N>
std::vector<int> recursiveBisection(const std::vector<Parfait::Point<double, N>>& points,
                                    int num_partitions,
                                    double tol) {
    std::vector<double> weights(points.size(), 1.0);
    return recursiveBisection(points, weights, num_partitions, tol);
}

template <typename T>
std::map<int, std::vector<T>> queueToOwners(const std::vector<T>& items, const std::vector<int>& owners) {
    std::map<int, std::vector<T>> queue;
    if (items.size() != owners.size()) PARFAIT_THROW("Cannot map from items to owners, mismatched size.");

    for (unsigned long i = 0; i < items.size(); i++) {
        queue[owners[i]].push_back(items[i]);
    }

    return queue;
}

template <size_t N>
std::vector<int> assignOnePerRank(MessagePasser mp, const std::vector<Parfait::Point<double, N>>& points) {
    auto points_per_rank = mp.Gather(points.size());
    int offset = 0;
    for (int i = 0; i < mp.Rank(); i++) offset += points_per_rank[i];
    std::vector<int> part(points.size());
    std::iota(part.begin(), part.end(), offset);
    return part;
}
template <size_t N>
std::vector<int> recursiveBisection(MessagePasser orig_mp,
                                    const std::vector<Parfait::Point<double, N>>& points,
                                    int num_partitions,
                                    double tol) {
    std::vector<double> weights(points.size(), 1.0);
    return recursiveBisection(orig_mp, points, weights, num_partitions, tol);
}

template <size_t N>
std::vector<int> recursiveBisection(MessagePasser orig_mp,
                                    std::vector<Parfait::Point<double, N>> points,
                                    std::vector<double> weights,
                                    int num_partitions,
                                    double tol) {
    if (num_partitions < orig_mp.NumberOfProcesses()) {
        PARFAIT_THROW("RCB cannot request fewer partitions than the number of ranks.");
    }
    long total_points = orig_mp.ParallelSum(points.size());
    if (total_points < orig_mp.NumberOfProcesses()) {
        return assignOnePerRank(orig_mp, points);
    }
    Parfait::wiggle(points);
    size_t i_own_this_many_points = points.size();
    auto comm = orig_mp.split(orig_mp.getCommunicator(), 0);
    auto mp = MessagePasser(comm);

    std::vector<int> node_owners(points.size(), orig_mp.Rank());
    std::vector<int> node_indices(points.size());
    std::iota(node_indices.begin(), node_indices.end(), 0);

    unsigned int leaf_id = 0;
    unsigned int depth = 0;
    while (mp.NumberOfProcesses() > 1) {
        auto domain = Parfait::ExtentBuilder::build<decltype(points), N>(points);
        domain = Parfait::ParallelExtent::getBoundingBox(mp, domain);
        int dimension = domain.longestDimension();
        auto center = domain.center()[dimension];
        double ratio = calcCutRatio(num_partitions);
        double c = Parfait::findCenter(mp, domain, points, weights, ratio, center, dimension, tol);

        std::vector<int> extract_left, extract_right;
        std::tie(extract_left, extract_right) = Parfait::splitAlongDimensionAt(points, dimension, c);

        auto points_left = Parfait::extract(points, extract_left);
        auto owners_left = Parfait::extract(node_owners, extract_left);
        auto indices_left = Parfait::extract(node_indices, extract_left);
        auto weights_left = Parfait::extract(weights, extract_left);
        extract_left.clear();
        extract_left.shrink_to_fit();

        auto points_right = Parfait::extract(points, extract_right);
        auto owners_right = Parfait::extract(node_owners, extract_right);
        auto indices_right = Parfait::extract(node_indices, extract_right);
        auto weights_right = Parfait::extract(weights, extract_right);
        extract_right.clear();
        extract_right.shrink_to_fit();

        int middle_rank = mp.NumberOfProcesses() / 2;
        points_left = mp.Balance(std::move(points_left), 0, middle_rank);
        weights_left = mp.Balance(std::move(weights_left), 0, middle_rank);
        owners_left = mp.Balance(std::move(owners_left), 0, middle_rank);
        indices_left = mp.Balance(std::move(indices_left), 0, middle_rank);

        points_right = mp.Balance(std::move(points_right), middle_rank, mp.NumberOfProcesses());
        weights_right = mp.Balance(std::move(weights_right), middle_rank, mp.NumberOfProcesses());
        owners_right = mp.Balance(std::move(owners_right), middle_rank, mp.NumberOfProcesses());
        indices_right = mp.Balance(std::move(indices_right), middle_rank, mp.NumberOfProcesses());

        int color;
        if (mp.Rank() < middle_rank) {
            color = 0;
            points = points_left;
            weights = weights_left;
            node_owners = owners_left;
            node_indices = indices_left;
            leaf_id = pow(2, depth) + leaf_id * 2;
            num_partitions = numberOfLeftPartitions(num_partitions);
        } else {
            color = 1;
            points = points_right;
            weights = weights_right;
            node_owners = owners_right;
            node_indices = indices_right;
            leaf_id = pow(2, depth) + (leaf_id * 2) + 1;
            num_partitions = numberOfRightPartitions(num_partitions);
        }

        auto new_comm = mp.split(mp.getCommunicator(), color);

        mp.destroyComm();

        mp = MessagePasser(new_comm);
        depth++;
    }
    mp.destroyComm();
    auto part_ids = recursiveBisection_serial(points, weights, leaf_id, depth, num_partitions, tol);

    auto outgoing_part_ids = queueToOwners(part_ids, node_owners);
    auto incoming_part_ids = orig_mp.Exchange(outgoing_part_ids);
    outgoing_part_ids.clear();

    auto outgoing_node_indices = queueToOwners(node_indices, node_owners);
    auto incoming_node_indices = orig_mp.Exchange(outgoing_node_indices);
    outgoing_node_indices.clear();

    part_ids = std::vector<int>(i_own_this_many_points, -1);
    auto incoming_ranks = orig_mp.NumberOfProcesses();
    for (auto r = 0; r < incoming_ranks; r++) {
        if (incoming_node_indices.count(r) == 0) continue;
        for (unsigned int i = 0; i < incoming_node_indices[r].size(); i++) {
            auto index = incoming_node_indices.at(r).at(i);
            auto part_id = incoming_part_ids.at(r).at(i);
            part_ids.at(index) = part_id;
        }
    }
    incoming_node_indices.clear();
    incoming_part_ids.clear();

    std::set<int> unique_ids(part_ids.begin(), part_ids.end());
    unique_ids = orig_mp.ParallelUnion(unique_ids);
    std::map<int, int> old_to_new;
    int next_new = 0;
    for (auto old : unique_ids) old_to_new[old] = next_new++;

    for (auto& p : part_ids) p = old_to_new.at(p);

    return part_ids;
}
}
