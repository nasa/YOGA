#pragma once
#include <string>
#include <set>
#include <parfait/Plane.h>
#include <parfait/PointWriter.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/Pipeline.h>
#include <t-infinity/TinfMesh.h>

namespace inf {

class EdgeCutTemplate {
  public:
    EdgeCutTemplate() = default;

    inline static EdgeCutTemplate cutSurfaceWithPlane(const inf::MeshInterface& mesh,
                                                      Parfait::Plane<double> plane,
                                                      std::set<int> surface_tags) {
        auto edges = inf::EdgeToNode::buildOnSurface(mesh, surface_tags);

        EdgeCutTemplate cut_template;
        for (auto& edge : edges) {
            auto p1 = mesh.node(edge[0]);
            auto p2 = mesh.node(edge[1]);
            double d = plane.edgeIntersectionWeight(p1, p2);
            if (d >= 0 and d <= 1.0) {
                cut_template.add(edge, {d, 1.0 - d});
            }
        }
        return cut_template;
    }

    inline void add(std::array<int, 2> nodes, std::array<double, 2> weights) {
        edge_nodes.push_back(nodes);
        edge_weights.push_back(weights);
    }

    template <typename GetFieldAtNode>
    std::vector<double> apply(GetFieldAtNode get_at_node) {
        std::vector<double> out(edge_nodes.size());
        for (int edge_index = 0; edge_index < int(edge_nodes.size()); edge_index++) {
            double d = 0;
            for (int i = 0; i < 2; i++) {
                d += edge_weights[edge_index][i] * get_at_node(edge_nodes[edge_index][i]);
            }
            out[edge_index] = d;
        }
        return out;
    }

    int numCutEdges() const { return int(edge_nodes.size()); }

    std::vector<Parfait::Point<double>> getCutPoints(const inf::MeshInterface& mesh) {
        std::vector<Parfait::Point<double>> points(numCutEdges());
        for (int e = 0; e < numCutEdges(); e++) {
            auto w = edge_weights[e];
            auto nodes = edge_nodes[e];
            Parfait::Point<double> p1 = mesh.node(nodes[0]);
            Parfait::Point<double> p2 = mesh.node(nodes[1]);
            points[e] = w[0] * p1 + w[1] * p2;
        }
        return points;
    }

  private:
    std::vector<std::array<int, 2>> edge_nodes;
    std::vector<std::array<double, 2>> edge_weights;
};

inline MeshAndFields surfacePlaneSample(
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields,
    Parfait::Plane<double> plane,
    std::set<int> surface_tags) {
    auto cut_template = inf::EdgeCutTemplate::cutSurfaceWithPlane(*mesh, plane, surface_tags);

    std::vector<std::shared_ptr<inf::FieldInterface>> out_fields;
    for (auto& f : fields) {
        if (f->association() != inf::FieldAttributes::Node()) {
            PARFAIT_THROW("Currently we can only take plane surface samples of NODE fields. " +
                          f->name() + " is associated with: " + f->association());
        }
        auto get_field_at_node = [&](int i) {
            double d;
            f->value(i, &d);
            return d;
        };
        auto cut_vector = cut_template.apply(get_field_at_node);

        auto f_as_class =
            std::make_shared<inf::VectorFieldAdapter>(f->name(), f->association(), 1, cut_vector);
        out_fields.push_back(f_as_class);
    }

    inf::TinfMeshData mesh_data;
    mesh_data.points = cut_template.getCutPoints(*mesh);

    long num_local_points = mesh_data.points.size();
    auto everyones_point_count = mp.Gather(num_local_points);
    long my_starting_gid = 0;
    for (int r = 0; r < mp.Rank(); r++) {
        my_starting_gid += everyones_point_count[r];
    }

    mesh_data.global_node_id.resize(num_local_points);
    auto b = mesh_data.global_node_id.begin();
    std::iota(b, b + num_local_points, my_starting_gid);

    mesh_data.node_owner.resize(num_local_points, mp.Rank());

    auto point_cloud = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());

    return {point_cloud, out_fields};
}

}
