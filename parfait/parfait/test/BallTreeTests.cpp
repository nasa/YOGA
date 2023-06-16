#include <parfait/KMeans.h>
#include <parfait/RecursiveBisection.h>
#include <parfait/STLFactory.h>
#include <parfait/Sphere.h>
#include <vector>

#include <RingAssertions.h>
#include <utility>

class BallTree {
  public:
    struct Node {
        Parfait::Sphere bounding_sphere;
        std::vector<int> inside;
        std::vector<int> children;
        int parent;
    };

    std::vector<Parfait::Sphere> bound(const std::vector<Parfait::Facet>& facets) {
        std::vector<Parfait::Sphere> spheres(facets.size());
        for (size_t i = 0; i < facets.size(); i++) {
            spheres[i] = Parfait::boundingSphere(facets[i]);
        }
        return spheres;
    }

    std::vector<int> partitionRBC(const std::vector<Parfait::Sphere>& spheres, int num_parts) const {
        std::vector<Parfait::Point<double>> centroids(spheres.size());
        for (size_t i = 0; i < spheres.size(); i++) {
            centroids[i] = spheres[i].center;
        }
        return Parfait::recursiveBisection(centroids, num_parts, 1.0e-4);
    }
    std::vector<int> partitionKMeans(const std::vector<Parfait::Sphere>& spheres, int num_parts) const {
        std::vector<Parfait::Point<double>> centroids(spheres.size());
        for (size_t i = 0; i < spheres.size(); i++) {
            centroids[i] = spheres[i].center;
        }
        return Parfait::KMeansClustering::apply(centroids, 8);
    }

    struct QueuedFacets {
        int node_id;
        std::vector<int> facet_ids;
    };
    BallTree(std::vector<Parfait::Facet> facets) : facets(std::move(facets)), bounding_spheres(bound(facets)) {
        nodes.resize(1);
        std::queue<QueuedFacets> to_process;
        std::vector<int> all_facets(facets.size());
        std::iota(all_facets.begin(), all_facets.end(), 0);
        to_process.push({0, all_facets});
        all_facets.clear();
        all_facets.shrink_to_fit();

        while (not to_process.empty()) {
            construct(to_process);
        }
    }

    void construct(std::queue<QueuedFacets>& to_process) {
        auto next_facets = to_process.front();
        to_process.pop();
        auto node = nodes[next_facets.node_id];
        if ((int)next_facets.facet_ids.size() <= max_size) {
            node.inside = next_facets.facet_ids;
            return;
        }

        printf("Processing %lu facets\n", next_facets.facet_ids.size());

        int num_parts = std::min(8, int(next_facets.facet_ids.size()));
        std::vector<Parfait::Sphere> spheres(next_facets.facet_ids.size());
        auto partitions = partitionRBC(spheres, num_parts);
        std::vector<std::vector<int>> partition_facets(num_parts);
        int next_child = nodes.size();
        nodes.resize(nodes.size() + num_parts);
        for (int i = 0; i < num_parts; i++) {
            to_process.push({next_child + i, partition_facets[i]});
            node.children.push_back(next_child + i);
            nodes[next_child + i].parent = next_facets.node_id;
        }
    }

  private:
    int max_size = 10;
    std::vector<Node> nodes;
    std::vector<Parfait::Facet> facets;
    std::vector<Parfait::Sphere> bounding_spheres;
};

template <typename T>
std::vector<std::vector<T>> split(const std::vector<T>& items, const std::vector<int>& colors, int num_colors) {
    std::vector<std::vector<T>> output;
    output.resize(num_colors);
    if (items.size() != colors.size()) {
        PARFAIT_THROW("Cannot split items by color if there isn't a color assigned for each item");
    }
    for (size_t i = 0; i < items.size(); i++) {
        output[colors[i]].push_back(items[i]);
    }
    return output;
}

TEST_CASE("random points k means bounding ball") {
    auto points = Parfait::generateRandomPoints(100);
    auto partition = Parfait::KMeansClustering::apply(points, 5);
    auto points_on_partition = split(points, partition, 5);
}
