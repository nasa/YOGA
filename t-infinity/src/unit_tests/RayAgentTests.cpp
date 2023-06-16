#include <parfait/Intersections.h>
#include <parfait/ToString.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>
#include <t-infinity/FaceNeighbors.h>
#include <RingAssertions.h>

namespace inf  {

struct FaceCrossing {
    std::array<int, 3> node_ids;
    std::array<double, 3> weights;
};

class RayAgent {
  public:
    inline RayAgent(const inf::MeshInterface& mesh, const std::vector<std::vector<int>>& c2f)
        : mesh(mesh), c2f(c2f) {}
    inline std::vector<FaceCrossing> walk(int starting_cell_id,
                                          Parfait::Point<double> origin,
                                          Parfait::Point<double> normal) {
        std::vector<FaceCrossing> crossings;
        int current_cell = starting_cell_id;
        int previous_cell = -1;
        while (current_cell >= 0) {
            inf::Cell cell(mesh, current_cell);
            FaceCrossing face_crossing;
            bool found_crossing = false;
            for (int face_number = 0; face_number < cell.faceCount(); face_number++) {
                auto points = cell.facePoints(face_number);
                auto nodes = cell.faceNodes(face_number);
                if (c2f[current_cell][face_number] == previous_cell) continue;
                if (rayIntersectsFace(points, nodes, origin, normal, face_crossing)) {
                    previous_cell = current_cell;
                    current_cell = c2f[current_cell][face_number];
                    crossings.push_back(face_crossing);
                    found_crossing = true;
                    break;
                }
            }

            if (not found_crossing) {
                return {};
            }
        }
        return crossings;
    }

  private:
    const inf::MeshInterface& mesh;
    const std::vector<std::vector<int>>& c2f;

    inline bool rayIntersectsFace(std::vector<Parfait::Point<double>>& points,
                                  const std::vector<int>& nodes,
                                  const Parfait::Point<double>& origin,
                                  const Parfait::Point<double>& normal,
                                  FaceCrossing& crossing) const {
        if (points.size() == 3) {
            auto weights = Parfait::Intersections::triangleAndRay(points[0], points[1], points[2], origin, normal);
            if (areWeightsValid(weights)) {
                crossing.weights = weights;
                crossing.node_ids[0] = nodes[0];
                crossing.node_ids[1] = nodes[1];
                crossing.node_ids[2] = nodes[2];
                return true;
            }
            return false;
        }
        if (points.size() == 4) {
            std::vector<Parfait::Point<double>> tri_points;
            tri_points.push_back(points[0]);
            tri_points.push_back(points[1]);
            tri_points.push_back(points[2]);
            std::vector<int> tri_nodes;
            tri_nodes.push_back(nodes[0]);
            tri_nodes.push_back(nodes[1]);
            tri_nodes.push_back(nodes[2]);
            if (rayIntersectsFace(tri_points, tri_nodes, origin, normal, crossing)) return true;
            tri_points[0] = points[1];
            tri_points[1] = points[3];
            tri_points[2] = points[2];
            tri_nodes[0] = nodes[1];
            tri_nodes[1] = nodes[3];
            tri_nodes[2] = nodes[2];
            return rayIntersectsFace(tri_points, tri_nodes, origin, normal, crossing);
        } else {
            PARFAIT_THROW("Encountered a face that wasn't a triangle or a quad");
        }
    }

    inline bool areWeightsValid(const std::array<double, 3>& weights) const {
        for (int i = 0; i < 3; i++)
            if (weights[i] < 0.0 or weights[i] > 1.0) return false;
        return true;
    }
};
}

TEST_CASE("Ray agent can walk on triangles") {
    auto mesh = inf::CartMesh::createVolume(5, 5, 5);
    auto c2f = inf::FaceNeighbors::build(*mesh);
    int starting_cell_id = 0;
    Parfait::Point<double> origin = {.1, .1, .1};
    Parfait::Point<double> normal = {.09, .09, .99};
    normal.normalize();
    inf::RayAgent agent(*mesh, c2f);
    auto crossings = agent.walk(starting_cell_id, origin, normal);
    REQUIRE(crossings.size() == 5);
}

TEST_CASE("Ray agent can walk on both triangles") {
    auto mesh = inf::CartMesh::createVolume(5, 5, 5);
    auto c2f = inf::FaceNeighbors::build(*mesh);
    int starting_cell_id = 0;
    Parfait::Point<double> origin = {.1, .1, .1};
    Parfait::Point<double> normal = {-.09, .09, .99};
    normal.normalize();
    inf::RayAgent agent(*mesh, c2f);
    auto crossings = agent.walk(starting_cell_id, origin, normal);
    REQUIRE(crossings.size() == 5);
}
