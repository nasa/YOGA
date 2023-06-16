#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Cell.h>
#include <t-infinity/Shortcuts.h>

class LukeMeshDef {
  public:
    inline static std::vector<std::vector<double>> calcDistFromVolumeAndSurface(const inf::MeshInterface& mesh,
                                                                                const std::vector<int>& surface_nodes) {
        std::vector<std::vector<double>> distances(mesh.nodeCount());
        for (auto& w : distances) {
            w.resize(surface_nodes.size());
        }

        for (int interior_node = 0; interior_node < mesh.nodeCount(); interior_node++) {
            Parfait::Point<double> p = mesh.node(interior_node);
            for (int i = 0; i < int(surface_nodes.size()); i++) {
                int boundary_node = surface_nodes[i];
                Parfait::Point<double> q = mesh.node(boundary_node);
                double dist = p.distance(q);
                distances.at(interior_node).at(i) = dist;
            }
        }

        return distances;
    }

    inline static std::vector<double> calcSurfaceNodeAreas(const inf::MeshInterface& mesh,
                                                           const std::vector<int>& surface_nodes) {
        std::vector<double> surface_node_areas(surface_nodes.size());
        auto n2c = inf::NodeToCell::buildSurfaceOnly(mesh);
        for (int i = 0; i < int(surface_nodes.size()); i++) {
            int node_id = surface_nodes[i];
            double node_area = 0.0;
            for (int neighbor_cell : n2c[node_id]) {
                inf::Cell cell(mesh, neighbor_cell);
                double face_area = cell.faceAreaNormal(0).magnitude();
                face_area /= (double)cell.nodes().size();
                node_area += face_area;
            }
            surface_node_areas[i] = node_area;
        }
        return surface_node_areas;
    }

    inline static std::vector<Parfait::Point<double>> calcInteriorDeflections(
        const inf::MeshInterface& mesh,
        const std::vector<std::vector<double>>& distances,
        const std::vector<double>& surface_node_areas,
        const std::vector<int>& surface_nodes,
        const std::vector<Parfait::Point<double>>& surface_deflections) {
        std::vector<Parfait::Point<double>> deflections(mesh.nodeCount());
        for(int i = 0; i < int(surface_nodes.size()); i++){
            int node_id = surface_nodes[i];
            deflections[node_id] = surface_deflections[i];
        }

        double Ldef = 0.5;  // "estimated length of the deformation"
        double a = 3.0;     // determined experimentally by Luke
        double b = 5.0;     // determined experimentally by Luke
        double nabla = 1;   // determined experimentally by Luke

        double total_node_area = 0.0;
        for(auto node_area : surface_node_areas){
            total_node_area += node_area;
        }

        Parfait::Point<double> average_deflection = {0,0,0};
        for(int i = 0; i < int(surface_nodes.size()); i++){
            average_deflection += surface_node_areas[i]*surface_deflections[i];
        }
        average_deflection /= total_node_area;

        double max_displacement_difference = 0.0;
        for(auto s : surface_deflections){
            double d = s.distance(average_deflection);
            max_displacement_difference = std::max(d, max_displacement_difference);
        }
        double alpha = nabla * max_displacement_difference / Ldef;
        alpha = std::max(alpha, 0.1);

        std::set<int> is_surface_node(surface_nodes.begin(), surface_nodes.end());

        int num_surface_nodes = surface_nodes.size();
        for(int node_id = 0; node_id < mesh.nodeCount(); node_id++){
            if(is_surface_node.count(node_id)) continue;
            double total_weight = 0.0;
            deflections[node_id] = {0,0,0};
            for(int i = 0; i < num_surface_nodes; i++){
                double d= distances[node_id][i];
                double w = surface_node_areas[i] * (std::pow(Ldef / d, a) + std::pow(alpha * Ldef / d, b));
                deflections[node_id] += surface_deflections[i] * w;
                total_weight += w;
            }
            deflections[node_id] /= total_weight;
        }


        return deflections;
    }

  public:
};

std::vector<Parfait::Point<double>> buildTestSurfaceDisplacements(MessagePasser mp,
                                                                  const inf::MeshInterface& mesh,
                                                                  const std::vector<int>& surface_nodes,
                                                                  const std::set<int>& critical_nodes,
                                                                  double amplitude) {
    // fake surface displacements with cosine function
    auto surface_displacement_function = [=](Parfait::Point<double> p) {
        Parfait::Point<double> centroid = {0.5, 0.5, 0.5};
        double d = p.distance(centroid);
        Parfait::Point<double> displacement = {0, 0, 0};
        displacement[2] = amplitude * exp(-1.0 / (1 - d));
        return displacement;
    };

    int bottom_tag = 1;
    auto surface_nodes_on_bottom_tag = inf::extractNodeIdsOn2DTags(mesh, bottom_tag);
    std::set<int> is_bottom_node(surface_nodes_on_bottom_tag.begin(), surface_nodes_on_bottom_tag.end());

    std::vector<Parfait::Point<double>> surface_deflections(surface_nodes.size());
    for (int i = 0; i < int(surface_nodes.size()); i++) {
        int node_id = surface_nodes[i];
        if (is_bottom_node.count(node_id) and not critical_nodes.count(node_id)) {
            Parfait::Point<double> p = mesh.node(node_id);
            surface_deflections[i] = surface_displacement_function(p);
        }
    }
    return surface_deflections;
}

void applySurfaceDeflections(inf::TinfMesh& mesh,
                             const std::vector<int>& surface_nodes,
                             const std::vector<Parfait::Point<double>>& surface_deflections) {
    auto& points = mesh.mesh.points;
    for (int i = 0; i < int(surface_nodes.size()); i++) {
        int surface_node_id = surface_nodes[i];
        points[surface_node_id] += surface_deflections[i];
    }
}

void applyDeflections(inf::TinfMesh& mesh,
                             const std::vector<Parfait::Point<double>>& deflections) {
    auto& points = mesh.mesh.points;
    for (int node_id = 0; node_id < mesh.nodeCount(); node_id++) {
        points[node_id] += deflections[node_id];
    }
}

std::set<int> findCriticalNodes(const inf::MeshInterface& mesh) {
    std::set<int> critical_nodes;
    auto all_surface_nodes = inf::extractAllNodeIdsIn2DCells(mesh);
    std::map<int, std::set<int>> nodes_in_tag;
    auto tags = inf::extractTagsWithDimensionality(mesh, 2);
    for(auto t : tags){
        auto tag_nodes = inf::extractNodeIdsOn2DTags(mesh, t);
        nodes_in_tag[t] = std::set<int>(tag_nodes.begin(), tag_nodes.end());
    }
    for(auto node_id : all_surface_nodes){
        int tag_count = 0;

        for(auto& pair : nodes_in_tag) {
            if(pair.second.count(node_id)){
                tag_count++;
                if(tag_count > 1) {
                    critical_nodes.insert(node_id);
                    break;
                }
            }
        }
    }
    return critical_nodes;
}

TEST_CASE("mesh deformation tests", "[Luke-mesh-def]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(10, 10, 10);
    auto surface_nodes = inf::extractAllNodeIdsIn2DCells(*mesh);
    auto distances = LukeMeshDef::calcDistFromVolumeAndSurface(*mesh, surface_nodes);
    REQUIRE(int(distances.size()) == mesh->nodeCount());

    auto critical_nodes = findCriticalNodes(*mesh);

    auto original_points = mesh->mesh.points;
    inf::shortcut::visualize("luke.0", mp, mesh);

    int num_steps = 10;
    double max_amp = 2.0;
    double dx = max_amp / num_steps;
    for(int step = 0; step < num_steps; step++){
        double amplitude = dx*step;
        mesh->mesh.points = original_points;
        auto surface_deflections = buildTestSurfaceDisplacements(mp, *mesh, surface_nodes, critical_nodes, amplitude);

        //    applySurfaceDeflections(*mesh, surface_nodes, surface_deflections);
        //    inf::shortcut::visualize("after-surface-deflections", mp, mesh);

        auto surface_node_areas = LukeMeshDef::calcSurfaceNodeAreas(*mesh, surface_nodes);
        auto node_deflections = LukeMeshDef::calcInteriorDeflections(
            *mesh, distances, surface_node_areas, surface_nodes, surface_deflections);

        applyDeflections(*mesh, node_deflections);
        inf::shortcut::visualize("luke." + std::to_string(step+1), mp, mesh);
    }
}

