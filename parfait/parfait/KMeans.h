#pragma once

#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/ExtentWriter.h>
#include <parfait/Point.h>
#include <parfait/PointWriter.h>
#include <chrono>
#include <random>
#include "Flatten.h"

namespace Parfait {

class KMeansClustering {
  public:
    inline static std::vector<int> apply(MessagePasser mp,
                                         const std::vector<Parfait::Point<double>>& points,
                                         int max_clusters,
                                         int max_iterations = -1) {
        auto assigned_cluster = roundRobinInitializeClusterId(points, max_clusters);
        auto cluster_centroids = initializeCentroidsToRandomPoints(mp, max_clusters, points);
        if (max_iterations < 0) max_iterations = std::numeric_limits<int>::max();
        int iter = 0;
        while (iter < max_iterations) {
            bool done = assignPointsToNearestCluster(points, cluster_centroids, assigned_cluster);
            done = mp.ParallelAnd(done);
            if (done) {
                break;
            }
            calcClusterCentroid(mp, points, assigned_cluster, cluster_centroids);
            iter++;
        }
        collapseClusterIdsToRemoveEmptyClusters(mp, assigned_cluster);
        return assigned_cluster;
    }
    inline static std::vector<int> apply(const std::vector<Parfait::Point<double>>& points,
                                         int max_clusters,
                                         int max_iterations = -1) {
        auto assigned_cluster = roundRobinInitializeClusterId(points, max_clusters);
        auto cluster_centroids = initializeCentroidsToRandomPoints(max_clusters, points);
        if (max_iterations < 0) max_iterations = std::numeric_limits<int>::max();

        int iter = 0;
        while (iter < max_iterations) {
            bool done = assignPointsToNearestCluster(points, cluster_centroids, assigned_cluster);
            if (done) break;
            calcClusterCentroid(points, assigned_cluster, cluster_centroids);
            iter++;
        }
        collapseClusterIdsToRemoveEmptyClusters(assigned_cluster);
        return assigned_cluster;
    }

  private:
    inline static std::vector<int> roundRobinInitializeClusterId(const std::vector<Parfait::Point<double>>& points,
                                                                 int num_clusters) {
        std::vector<int> cluster_id(points.size());
        int current_cluster = 0;
        for (size_t n = 0; n < points.size(); n++) {
            cluster_id[n] = current_cluster++;
            current_cluster = current_cluster % num_clusters;
        }
        return cluster_id;
    }
    inline static std::vector<Parfait::Point<double>> calcClusterCentroid(
        const std::vector<Parfait::Point<double>>& points,
        const std::vector<int>& assigned_cluster,
        std::vector<Parfait::Point<double>>& cluster_centroids) {
        std::vector<int> cluster_point_count(cluster_centroids.size(), 0);
        for (auto& c : cluster_centroids) {
            c = Point<double>{0, 0, 0};
        }
        for (size_t n = 0; n < points.size(); n++) {
            int cluster_id = assigned_cluster[n];
            cluster_centroids[cluster_id] += points[n];
            cluster_point_count[cluster_id]++;
        }
        for (size_t c = 0; c < cluster_point_count.size(); c++) {
            auto count = static_cast<double>(cluster_point_count[c]);
            if (cluster_point_count[c] == 0) {
                count = 0.0;
            }
            cluster_centroids[c] /= count;
        }

        return cluster_centroids;
    }

    inline static std::vector<Parfait::Point<double>> calcClusterCentroid(
        MessagePasser mp,
        const std::vector<Parfait::Point<double>>& points,
        const std::vector<int>& assigned_cluster,
        std::vector<Parfait::Point<double>>& cluster_centroids) {
        std::vector<int> cluster_point_count(cluster_centroids.size(), 0);
        int num_clusters = int(cluster_centroids.size());
        for (auto& c : cluster_centroids) {
            c = Point<double>{0, 0, 0};
        }
        for (size_t n = 0; n < points.size(); n++) {
            int cluster_id = assigned_cluster[n];
            cluster_centroids[cluster_id] += points[n];
            cluster_point_count[cluster_id]++;
        }
        for (int c = 0; c < num_clusters; c++) {
            cluster_point_count[c] = mp.ParallelSum(cluster_point_count[c]);
            cluster_centroids[c][0] = mp.ParallelSum(cluster_centroids[c][0]);
            cluster_centroids[c][1] = mp.ParallelSum(cluster_centroids[c][1]);
            cluster_centroids[c][2] = mp.ParallelSum(cluster_centroids[c][2]);
        }
        for (size_t c = 0; c < cluster_point_count.size(); c++) {
            auto count = static_cast<double>(cluster_point_count[c]);
            if (cluster_point_count[c] == 0) {
                count = 0.0;
            }
            cluster_centroids[c] /= count;
        }

        return cluster_centroids;
    }

    inline static int calcClosestId(const Point<double>& p, const std::vector<Parfait::Point<double>>& centroids) {
        int closest = 0;
        double min_distance = Parfait::Point<double>::distance(p, centroids[0]);
        for (size_t c = 1; c < centroids.size(); c++) {
            double d = Parfait::Point<double>::distance(p, centroids[c]);
            if (d < min_distance) {
                min_distance = d;
                closest = c;
            }
        }
        return closest;
    }

    inline static bool assignPointsToNearestCluster(const std::vector<Parfait::Point<double>>& points,
                                                    const std::vector<Parfait::Point<double>>& centroids,
                                                    std::vector<int>& cluster) {
        bool done = true;
        for (size_t n = 0; n < points.size(); n++) {
            int current_cluster = cluster[n];
            int closest_cluster = calcClosestId(points[n], centroids);
            if (current_cluster != closest_cluster) done = false;
            cluster[n] = closest_cluster;
        }
        return done;
    }
    inline static std::vector<Parfait::Point<double>> initializeCentroidsToRandomPoints(
        MessagePasser mp, int num_clusters, const std::vector<Parfait::Point<double>>& points) {
        std::vector<Parfait::Point<double>> cluster_centroids;
        if (num_clusters < (int)points.size()) {
            cluster_centroids.resize(num_clusters);
            for (int c = 0; c < num_clusters; c++) {
                int random_point = rand() % points.size();
                cluster_centroids[c] = points[random_point];
            }
        }

        cluster_centroids = Parfait::flatten(mp.Gather(cluster_centroids, 0));
        std::mt19937 gen;
        gen.seed(42);
        std::shuffle(cluster_centroids.begin(), cluster_centroids.end(), gen);
        cluster_centroids.resize(num_clusters);

        mp.Broadcast(cluster_centroids, 0);
        return cluster_centroids;
    }
    inline static std::vector<Parfait::Point<double>> initializeCentroidsToRandomPoints(
        int num_clusters, const std::vector<Parfait::Point<double>>& points) {
        std::vector<Parfait::Point<double>> cluster_centroids(num_clusters);
        for (int c = 0; c < num_clusters; c++) {
            int random_point = rand() % points.size();
            cluster_centroids[c] = points[random_point];
        }
        return cluster_centroids;
    }
    inline static void collapseClusterIdsToRemoveEmptyClusters(std::vector<int>& assigned_cluster) {
        std::map<int, int> old_to_new;
        int next_cluster_id = 0;
        for (const auto& c : assigned_cluster) {
            if (old_to_new.count(c) == 0) {
                old_to_new[c] = next_cluster_id++;
            }
        }

        for (auto& c : assigned_cluster) {
            c = old_to_new.at(c);
        }
    }

    inline static void collapseClusterIdsToRemoveEmptyClusters(MessagePasser mp, std::vector<int>& assigned_cluster) {
        int next_cluster_id = 0;

        std::set<int> unique_clusters_remaining(assigned_cluster.begin(), assigned_cluster.end());
        unique_clusters_remaining = mp.ParallelUnion(unique_clusters_remaining);

        std::map<int, int> old_to_new;
        for (const auto& c : unique_clusters_remaining) {
            if (old_to_new.count(c) == 0) {
                old_to_new[c] = next_cluster_id++;
            }
        }

        for (auto& c : assigned_cluster) {
            c = old_to_new.at(c);
        }
    }
};

inline int countNumUniqueCategories(const std::vector<int>& categories) {
    std::set<int> unique;
    for (auto c : categories) unique.insert(c);
    return static_cast<int>(unique.size());
}

inline std::vector<Parfait::Extent<double>> createExtentsForCategories(
    const std::vector<Parfait::Point<double>>& points, const std::vector<int>& ids) {
    int num_categories = countNumUniqueCategories(ids);
    std::vector<Parfait::Extent<double>> extents(num_categories);
    for (auto& e : extents) {
        e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    }
    for (size_t i = 0; i < points.size(); i++) {
        Parfait::ExtentBuilder::add(extents[ids[i]], points[i]);
    }
    return extents;
}

}
