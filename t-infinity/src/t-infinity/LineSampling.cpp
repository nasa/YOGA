#include "LineSampling.h"
#include <t-infinity/VectorFieldAdapter.h>
#include <parfait/DataFrame.h>
#include <parfait/Flatten.h>
#include <parfait/LinePlotters.h>
#include <parfait/Linspace.h>
#include <Tracer.h>
void inf::linesampling::Cut::addTriIfIntersects(int node0, int node1, int node2) {
    Parfait::Point<double> a = mesh.node(node0);
    Parfait::Point<double> b = mesh.node(node1);
    Parfait::Point<double> c = mesh.node(node2);
    Parfait::Facet f(a, b, c);
    Parfait::Point<double> pierce_point;
    double total_length = (line_b - line_a).magnitude();
    if (f.WhereDoesEdgeIntersect(line_a, line_b, pierce_point)) {
        auto weights = Parfait::calcBaryCentricWeights(a, b, c, pierce_point);
        double pierce_length = (pierce_point - line_a).magnitude();
        double t = pierce_length / total_length;
        node_t_value.push_back(t);
        node_weights.push_back(weights);
        node_donor_nodes.push_back(std::array<int, 3>{node0, node1, node2});
    }
}
void inf::linesampling::Cut::performCut(const std::vector<std::vector<int>>& face_neighbors,
                                        const std::vector<int>& cell_id_set) {
    for (const auto& c : cell_id_set) {
        if (mesh.cellOwner(c) != mesh.partitionId()) continue;
        inf::Cell cell(mesh, c);
        auto vertices = cell.points();
        auto cell_extent = Parfait::ExtentBuilder::build(vertices);
        if (not cell_extent.intersectsSegment(line_a, line_b)) continue;

        int num_faces = cell.faceCount();
        for (int i = 0; i < num_faces; i++) {
            int neighbor = face_neighbors[c][i];
            if (c > neighbor) continue;  // only peform the sampling once per face
            auto face_nodes = cell.faceNodes(i);
            if (face_nodes.size() == 3) {
                addTriIfIntersects(face_nodes[0], face_nodes[1], face_nodes[2]);
            } else if (face_nodes.size() == 4) {
                addTriIfIntersects(face_nodes[0], face_nodes[1], face_nodes[2]);
                addTriIfIntersects(face_nodes[0], face_nodes[2], face_nodes[3]);
            } else {
                PARFAIT_THROW(
                    "Encountered a face with more than 4 nodes...Neat! But not implemented.");
            }
        }
    }
}
std::vector<Parfait::Point<double>> inf::linesampling::Cut::getSampledPoints() const {
    std::vector<Parfait::Point<double>> points;
    for (int n = 0; n < nodeCount(); n++) {
        Parfait::Point<double> p = {0, 0, 0};
        for (int i = 0; i < 3; i++) {
            int node = node_donor_nodes[n][i];
            Parfait::Point<double> mesh_p = mesh.node(node);
            p += node_weights[n][i] * mesh_p;
        }
        points.push_back(p);
    }
    return points;
}
std::vector<double> inf::linesampling::Cut::apply(const std::vector<double>& f) const {
    std::vector<double> output(nodeCount());
    for (int n = 0; n < nodeCount(); n++) {
        double d = 0.0;
        for (int i = 0; i < 3; i++) {
            int node = node_donor_nodes[n][i];
            d += node_weights[n][i] * f[node];
        }
        output[n] = d;
    }
    return output;
}
std::shared_ptr<inf::FieldInterface> inf::linesampling::Cut::apply(
    const inf::FieldInterface& f) const {
    std::vector<double> output(nodeCount());
    for (int n = 0; n < nodeCount(); n++) {
        double d = 0.0;
        for (int i = 0; i < 3; i++) {
            int node = node_donor_nodes[n][i];
            double fd;
            f.value(node, &fd);
            d += node_weights[n][i] * fd;
        }
        output.push_back(d);
    }
    return std::make_shared<inf::VectorFieldAdapter>(
        f.name(), inf::FieldAttributes::Node(), output);
}
void inf::linesampling::visualize(std::string filename,
                                  MessagePasser mp,
                                  const inf::MeshInterface& mesh,
                                  Parfait::Point<double> a,
                                  Parfait::Point<double> b,
                                  const std::vector<std::shared_ptr<FieldInterface>>& fields) {}

Parfait::DataFrame inf::linesampling::sample(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             const std::vector<std::vector<int>>& face_neighbors,
                                             const std::vector<int>& cell_id_set,
                                             Parfait::Point<double> a,
                                             Parfait::Point<double> b,
                                             const std::vector<std::vector<double>>& node_field,
                                             const std::vector<std::string>& field_names) {
    Tracer::begin("Sort cell id set");
    std::vector<int> sorted_cell_id_set(std::move(cell_id_set));
    std::sort(sorted_cell_id_set.begin(), sorted_cell_id_set.end());
    sorted_cell_id_set.erase( std::unique( sorted_cell_id_set.begin(), sorted_cell_id_set.end() ), sorted_cell_id_set.end() );
    Tracer::end("Sort cell id set");

    Tracer::begin("Perform mesh cut");
    Cut cut(mesh, face_neighbors, sorted_cell_id_set, a, b);
    Tracer::end("Perform mesh cut");

    Tracer::begin("Bring samples to rank 0");
    Parfait::DataFrame df;
    auto global_points = Parfait::flatten(mp.Gather(cut.getSampledPoints(), 0));
    if (mp.Rank() == 0) {
        std::vector<double> x(global_points.size());
        std::vector<std::string> names = {"x", "y", "z"};
        for (int i = 0; i < 3; i++) {
            for (size_t n = 0; n < global_points.size(); n++) {
                x[n] = global_points[n][i];
            }
            df[names[i]] = x;
        }
    }
    global_points.clear();
    global_points.shrink_to_fit();

    for (size_t i = 0; i < node_field.size(); i++) {
        auto& f = node_field[i];
        auto& f_name = field_names[i];
        auto global_field = Parfait::flatten(mp.Gather(cut.apply(f), 0));
        if (mp.Rank() == 0) {
            df[f_name] = global_field;
        }
    }
    Tracer::end("Bring samples to rank 0");

    return df;
}

Parfait::DataFrame inf::linesampling::sample(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             Parfait::Point<double> a,
                                             Parfait::Point<double> b,
                                             const std::vector<std::vector<double>>& node_field,
                                             const std::vector<std::string>& field_names) {
    auto face_neighbors = inf::FaceNeighbors::build(mesh);
    return sample(mp, mesh, face_neighbors, a, b, node_field, field_names);
}

Parfait::DataFrame inf::linesampling::sample(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             const std::vector<std::vector<int>>& face_neighbors,
                                             Parfait::Point<double> a,
                                             Parfait::Point<double> b,
                                             const std::vector<std::vector<double>>& node_field,
                                             const std::vector<std::string>& field_names) {
    std::vector<int> cell_id_set;
    for (auto c = 0; c < mesh.cellCount(); c++) cell_id_set.push_back(c);
    return sample(mp, mesh, face_neighbors, cell_id_set, a, b, node_field, field_names);
}

Parfait::DataFrame inf::linesampling::sample(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             const std::vector<std::vector<int>>& face_neighbors,
                                             const Parfait::Adt3DExtent& adt,
                                             Parfait::Point<double> a,
                                             Parfait::Point<double> b,
                                             const std::vector<std::vector<double>>& node_field,
                                             const std::vector<std::string>& field_names) {
    
    Tracer::begin("Line subdivision");
    // get the longest diagonal adt bounding volume length
    auto adt_extent_corners = adt.boundingExtent().corners();
    double adt_extent_max_length = std::numeric_limits<double>::min();
    for (const auto& c1 : adt_extent_corners) {
        for (const auto& c2 : adt_extent_corners) {
            auto dist = c1.distance(c2);
            if (dist > adt_extent_max_length) adt_extent_max_length = dist;
        }
    }
    // subdivide the line if necessary
    int max_segments = 201;
    auto segment_length = adt_extent_max_length / (max_segments + 1);
    auto overall_line_length = a.distance(b);
    int n_segments = static_cast<int>(std::ceil(overall_line_length / segment_length));
    n_segments = std::min(n_segments, max_segments);
    n_segments = std::max(1, n_segments);
    auto segment_boundaries = Parfait::linspace(0.0, 1.0, n_segments + 1);
    // get the extent of each line segment and retrieve cell ids
    std::vector<int> overall_cell_id_vector(mesh.cellCount());
    Tracer::end("Line subdivision");
    for (int i = 0; i < n_segments; i++) {
        Tracer::begin("Line segment extent setup");
        double eps = 1e-4;
        auto ta = segment_boundaries[i];
        auto tb = segment_boundaries[i + 1];
        if (i > 0) ta -= eps;
        if (i <= n_segments) tb += eps;
        std::vector<Parfait::Point<double>> segment_points;
        segment_points.push_back(a + ta * (b - a));
        segment_points.push_back(a + tb * (b - a));
        auto line_segment_extent = Parfait::ExtentBuilder::build(segment_points);
        Tracer::end("Line segment extent setup");
        Tracer::begin("ADT Retrieval");
        auto cell_id_set = adt.retrieve(line_segment_extent);
        Tracer::end("ADT Retrieval");
        Tracer::begin("Add cell ids to set");
        overall_cell_id_vector.insert(overall_cell_id_vector.end(), cell_id_set.begin(), cell_id_set.end());
        Tracer::end("Add cell ids to set");
    }
    
    return sample(mp, mesh, face_neighbors, overall_cell_id_vector, a, b, node_field, field_names);
}
