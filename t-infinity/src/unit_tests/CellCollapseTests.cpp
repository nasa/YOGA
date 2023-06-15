#include <RingAssertions.h>
#include <set>
#include <t-infinity/MeshInterface.h>
#include <parfait/ToString.h>
#include <parfait/Throw.h>
#include <t-infinity/Cell.h>

namespace inf  {
namespace CellCollapse {
    template <typename T>
    bool all_same(const std::vector<T>& things) {
        if (things.empty()) return true;
        const auto first = things.front();
        for (const auto& t : things) {
            if (t != first) return false;
        }
        return true;
    }
    void collapse(int* cell_nodes, inf::MeshInterface::CellType& type) {}

    void triangle(int* tri_nodes, inf::MeshInterface::CellType& type) {
        type = inf::MeshInterface::TRI_3;
        int num_unique = int(std::set<int>(tri_nodes, tri_nodes + 3).size());

        if (num_unique == 3) return;

        if (num_unique == 2) {
            if (tri_nodes[0] == tri_nodes[1]) {
                tri_nodes[1] = tri_nodes[2];
                tri_nodes[2] = -1;
                type = inf::MeshInterface::BAR_2;
                return;
            }
            if (tri_nodes[1] == tri_nodes[2]) {
                tri_nodes[2] = -1;
                type = inf::MeshInterface::BAR_2;
                return;
            }
            if (tri_nodes[0] == tri_nodes[2]) {
                tri_nodes[2] = -1;
                type = inf::MeshInterface::BAR_2;
                return;
            }
        }

        if (num_unique == 1) {
            tri_nodes[1] = -1;
            tri_nodes[2] = -1;
            type = inf::MeshInterface::NODE;
            return;
        }
        std::vector<int> nodes_vec(tri_nodes, tri_nodes + 3);
        PARFAIT_THROW("Unknown Tri collapse case encountered: " + Parfait::to_string(nodes_vec));
    }

    void quad(int* nodes, inf::MeshInterface::CellType& type) {
        type = inf::MeshInterface::QUAD_4;
        int num_unique = int(std::set<int>(nodes, nodes + 4).size());
        if (num_unique == 4) return;
        if (num_unique == 1) {
            nodes[1] = -1;
            nodes[2] = -1;
            nodes[3] = -1;
            type = inf::MeshInterface::NODE;
            return;
        }

        if (num_unique == 3 or num_unique == 2) {
            if (nodes[0] == nodes[1]) {
                nodes[1] = nodes[2];
                nodes[2] = nodes[3];
                nodes[3] = -1;
                type = inf::MeshInterface::TRI_3;
                if (num_unique == 3) return;
                triangle(nodes, type);
                return;
            }
            if (nodes[1] == nodes[2]) {
                nodes[2] = nodes[3];
                nodes[3] = -1;
                type = inf::MeshInterface::TRI_3;
                if (num_unique == 3) return;
                triangle(nodes, type);
                return;
            }
            if (nodes[2] == nodes[3] or nodes[0] == nodes[3]) {
                nodes[3] = -1;
                type = inf::MeshInterface::TRI_3;
                if (num_unique == 3) return;
                triangle(nodes, type);
                return;
            }
            if (nodes[0] == nodes[2] or nodes[1] == nodes[3]) {
                nodes[0] = -1;
                nodes[1] = -1;
                nodes[2] = -1;
                nodes[3] = -1;
                type = inf::MeshInterface::CellType(-1);
                return;
            }
        }
        std::vector<int> nodes_vec(nodes, nodes + 4);
        PARFAIT_THROW("Unknown Quad collapse case encountered: " + Parfait::to_string(nodes_vec));
    }
    void tet(int* nodes, inf::MeshInterface::CellType& type) {
        type = inf::MeshInterface::TETRA_4;
        int num_unique = int(std::set<int>(nodes, nodes + 4).size());
        if (num_unique == 4) return;

        if (num_unique == 3) {
            if (nodes[0] == nodes[1]) {
                nodes[1] = nodes[2];
                nodes[2] = nodes[3];
                nodes[3] = -1;
                type = inf::MeshInterface::CellType::TRI_3;
                return;
            }
            if (nodes[0] == nodes[2] or nodes[1] == nodes[2]) {
                nodes[2] = nodes[3];
                nodes[3] = -1;
                type = inf::MeshInterface::CellType::TRI_3;
                return;
            }
            if (nodes[3] == nodes[0] or nodes[3] == nodes[1] or nodes[3] == nodes[2]) {
                nodes[3] = -1;
                type = inf::MeshInterface::CellType::TRI_3;
                return;
            }
        }

        if (num_unique == 1) {
            nodes[1] = nodes[2] = nodes[3] = -1;
            type = inf::MeshInterface::CellType::NODE;
        }

        if (num_unique == 2) {
            PARFAIT_THROW("Unknown Tet collapse to BAR not implemented");
        }

        std::vector<int> nodes_vec(nodes, nodes + 4);
        PARFAIT_THROW("Unknown Tet collapse case encountered: Tet to edge not implemented?" +
                      Parfait::to_string(nodes_vec));
    }
    void prism(int* nodes, inf::MeshInterface::CellType& type) {
        type = inf::MeshInterface::PENTA_6;
        int num_unique = int(std::set<int>(nodes, nodes + 6).size());
        if (num_unique == 6) return;

        if (num_unique == 1) {
            nodes[1] = nodes[2] = nodes[3] = nodes[4] = nodes[5] = -1;
            type = inf::MeshInterface::NODE;
        }

        if (num_unique == 5) {
            if (nodes[1] == nodes[4]) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[0];
                nodes[1] = temp[2];
                nodes[2] = temp[5];
                nodes[3] = temp[3];
                nodes[4] = temp[4];
                nodes[5] = -1;
                type = inf::MeshInterface::PYRA_5;
                return;
            } else if (nodes[2] == nodes[5]) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[1];
                nodes[1] = temp[0];
                nodes[2] = temp[3];
                nodes[3] = temp[4];
                nodes[4] = temp[2];
                nodes[5] = -1;
                type = inf::MeshInterface::PYRA_5;
                return;
            } else if (nodes[0] == nodes[3]) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[1];
                nodes[1] = temp[4];
                nodes[2] = temp[5];
                nodes[3] = temp[2];
                nodes[4] = temp[0];
                nodes[5] = -1;
                type = inf::MeshInterface::PYRA_5;
                return;
            } else {
                nodes[0] = nodes[1] = nodes[2] = nodes[3] = nodes[4] = nodes[5] = -1;
                type = inf::MeshInterface::CellType(-1);
                return;
            }
        }

        if (num_unique == 4) {
            if (all_same(std::vector<int>{nodes[0], nodes[1], nodes[2]})) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[3];
                nodes[1] = temp[5];
                nodes[2] = temp[4];
                nodes[3] = temp[0];
                nodes[4] = -1;
                nodes[5] = -1;
                type = inf::MeshInterface::TETRA_4;
                return;
            }
            if (all_same(std::vector<int>{nodes[3], nodes[4], nodes[5]})) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[0];
                nodes[1] = temp[1];
                nodes[2] = temp[2];
                nodes[3] = temp[3];
                nodes[4] = -1;
                nodes[5] = -1;
                type = inf::MeshInterface::TETRA_4;
                return;
            }
            if (all_same(std::vector<int>{nodes[1], nodes[2], nodes[4], nodes[5]})) {
                std::vector<int> temp(nodes, nodes + 6);
                nodes[0] = temp[5];
                nodes[1] = temp[3];
                nodes[2] = temp[0];
                nodes[3] = temp[1];
                nodes[4] = -1;
                nodes[5] = -1;
                type = inf::MeshInterface::TETRA_4;
                return;
            }
        }
        if (num_unique == 3) {
            PARFAIT_THROW("Prism collapse to Tri case encountered: not implemented?");
        }
        if (num_unique == 2) {
            PARFAIT_THROW("Prism collapse to Edge case encountered: not implemented?");
        }

        std::vector<int> nodes_vec(nodes, nodes + 6);
        PARFAIT_THROW("Unknown Prism collapse case encountered: Prism nodes" + Parfait::to_string(nodes_vec));
    }

    void hex(int* nodes, inf::MeshInterface::CellType& type) {
        type = inf::MeshInterface::HEXA_8;
        int num_unique = int(std::set<int>(nodes, nodes + 8).size());
        if (num_unique == 8) return;
        if (num_unique == 1) {
            nodes[1] = nodes[2] = nodes[3] = nodes[4] = nodes[5] = nodes[6] = nodes[7];
            type = inf::MeshInterface::NODE;
            return;
        }

        if(num_unique == 6){
            type = inf::MeshInterface::PENTA_6;
            if(nodes[0] == nodes[1] and nodes[2] == nodes[3]){
                std::vector<int> temp(nodes, nodes + 8);
                nodes[0] = temp[0];
                nodes[1] = temp[4];
                nodes[2] = temp[5];
                nodes[3] = temp[2];
                nodes[4] = temp[7];
                nodes[5] = temp[6];
                nodes[6] = -1;
                nodes[7] = -1;
                return;
            }
            if(nodes[1] == nodes[5] and nodes[2] == nodes[6]){
                std::vector<int> temp(nodes, nodes + 8);
                nodes[0] = temp[0];
                nodes[1] = temp[4];
                nodes[2] = temp[1];
                nodes[3] = temp[3];
                nodes[4] = temp[7];
                nodes[5] = temp[2];
                nodes[6] = -1;
                nodes[7] = -1;
                return;
            }
            if(nodes[0] == nodes[4] and nodes[3] == nodes[7]){
                std::vector<int> temp(nodes, nodes + 8);
                nodes[0] = temp[0];
                nodes[1] = temp[5];
                nodes[2] = temp[1];
                nodes[3] = temp[3];
                nodes[4] = temp[6];
                nodes[5] = temp[2];
                nodes[6] = -1;
                nodes[7] = -1;
                return;
            }
        }
        std::vector<int> nodes_vec(nodes, nodes + 8);
        PARFAIT_THROW("Unknown Hex collapse case encountered: nodes: " + Parfait::to_string(nodes_vec));
    }

}
}

template <typename T>
bool all_equal(const std::vector<T>& things, const T& target) {
    for (const auto& t : things) {
        if (t != target) return false;
    }
    return true;
}

TEST_CASE("Hex") {
    inf::MeshInterface::CellType type = inf::MeshInterface::HEXA_8;
    SECTION("Good Hex") {
        std::vector<int> nodes = {2, 3, 4, 5, 6, 7, 8, 9};
        inf::CellCollapse::hex(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::HEXA_8);
        REQUIRE(nodes[0] == 2);
        REQUIRE(nodes[1] == 3);
        REQUIRE(nodes[2] == 4);
        REQUIRE(nodes[3] == 5);
        REQUIRE(nodes[4] == 6);
        REQUIRE(nodes[5] == 7);
        REQUIRE(nodes[6] == 8);
        REQUIRE(nodes[7] == 9);
    }
    SECTION("Degenerate hex") {
        std::vector<int> nodes = {2, 2, 2, 2, 2, 2, 2, 2};
        inf::CellCollapse::hex(nodes.data(), type);
        REQUIRE(all_equal(nodes, 2));
    }
    SECTION("Face Collapse to Edge 0,1 and 2,3"){
        std::vector<int> nodes = {2, 2, 4, 4, 6, 7, 8, 9};
        inf::CellCollapse::hex(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PENTA_6);
        REQUIRE(nodes[0] == 2);
        REQUIRE(nodes[1] == 6);
        REQUIRE(nodes[2] == 7);
        REQUIRE(nodes[3] == 4);
        REQUIRE(nodes[4] == 9);
        REQUIRE(nodes[5] == 8);
        REQUIRE(nodes[6] == -1);
        REQUIRE(nodes[7] == -1);
    }
    SECTION("Face Collapse to Edge 1,5 and 2,6"){
        std::vector<int> nodes = {2, 3, 4, 5, 6, 3, 4, 9};
        inf::CellCollapse::hex(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PENTA_6);
        REQUIRE(nodes[0] == 2);
        REQUIRE(nodes[1] == 6);
        REQUIRE(nodes[2] == 3);
        REQUIRE(nodes[3] == 5);
        REQUIRE(nodes[4] == 9);
        REQUIRE(nodes[5] == 4);
        REQUIRE(nodes[6] == -1);
        REQUIRE(nodes[7] == -1);
    }
    SECTION("Face Collapse to Edge 0,4 and 3,7"){
        std::vector<int> nodes = {2, 3, 4, 5, 2, 7, 8, 5};
        inf::CellCollapse::hex(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PENTA_6);
        REQUIRE(nodes[0] == 2);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 3);
        REQUIRE(nodes[3] == 5);
        REQUIRE(nodes[4] == 8);
        REQUIRE(nodes[5] == 4);
        REQUIRE(nodes[6] == -1);
        REQUIRE(nodes[7] == -1);
    }
}

TEST_CASE("Prism") {
    inf::MeshInterface::CellType type = inf::MeshInterface::PENTA_6;
    SECTION("Good Prism") {
        std::vector<int> nodes = {0, 1, 2, 3, 4, 5};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(nodes[0] == 0);
        REQUIRE(nodes[1] == 1);
        REQUIRE(nodes[2] == 2);
        REQUIRE(nodes[3] == 3);
        REQUIRE(nodes[4] == 4);
        REQUIRE(nodes[5] == 5);
        REQUIRE(type == inf::MeshInterface::PENTA_6);
    }
    SECTION("Edge collapse 1-4") {
        std::vector<int> nodes = {4, 5, 6, 7, 5, 9};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PYRA_5);
        REQUIRE(nodes[0] == 4);
        REQUIRE(nodes[1] == 6);
        REQUIRE(nodes[2] == 9);
        REQUIRE(nodes[3] == 7);
        REQUIRE(nodes[4] == 5);
        REQUIRE(nodes[5] == -1);
    }
    SECTION("Edge collapse 2-5") {
        std::vector<int> nodes = {4, 5, 6, 7, 8, 6};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PYRA_5);
        REQUIRE(nodes[0] == 5);
        REQUIRE(nodes[1] == 4);
        REQUIRE(nodes[2] == 7);
        REQUIRE(nodes[3] == 8);
        REQUIRE(nodes[4] == 6);
        REQUIRE(nodes[5] == -1);
    }
    SECTION("Edge collapse 0-3") {
        std::vector<int> nodes = {4, 5, 6, 4, 8, 9};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::PYRA_5);
        REQUIRE(nodes[0] == 5);
        REQUIRE(nodes[1] == 8);
        REQUIRE(nodes[2] == 9);
        REQUIRE(nodes[3] == 6);
        REQUIRE(nodes[4] == 4);
        REQUIRE(nodes[5] == -1);
    }
    SECTION("Edge collapse 1-2") {
        std::vector<int> nodes = {4, 5, 5, 7, 8, 9};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::CellType(-1));
        REQUIRE(all_equal(nodes, -1));
    }
    SECTION("Edge collapse 2-3") {
        std::vector<int> nodes = {4, 5, 4, 7, 8, 9};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::CellType(-1));
        REQUIRE(all_equal(nodes, -1));
    }
    // There are other edge collapse cases.  But they should all be caught by existing code

    SECTION("Face collapse 0,1,2") {
        std::vector<int> nodes = {4, 4, 4, 7, 8, 9};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::TETRA_4);
        REQUIRE(nodes[0] == 7);
        REQUIRE(nodes[1] == 9);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == 4);
        REQUIRE(nodes[4] == -1);
        REQUIRE(nodes[5] == -1);
    }

    SECTION("Face collapse 3,4,5") {
        std::vector<int> nodes = {4, 5, 6, 7, 7, 7};
        inf::CellCollapse::prism(nodes.data(), type);
        REQUIRE(type == inf::MeshInterface::TETRA_4);
        REQUIRE(nodes[0] == 4);
        REQUIRE(nodes[1] == 5);
        REQUIRE(nodes[2] == 6);
        REQUIRE(nodes[3] == 7);
        REQUIRE(nodes[4] == -1);
        REQUIRE(nodes[5] == -1);
    }

    SECTION("Face collapse 1,2,4,5") {
        std::vector<int> nodes = {4, 5, 5, 7, 5, 5};
        REQUIRE_THROWS(inf::CellCollapse::prism(nodes.data(), type));
    }
}

TEST_CASE("Tet") {
    inf::MeshInterface::CellType type = inf::MeshInterface::TETRA_4;
    SECTION("Good Tet") {
        std::vector<int> nodes = {0, 1, 2, 3};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 0);
        REQUIRE(nodes[1] == 1);
        REQUIRE(nodes[2] == 2);
        REQUIRE(nodes[3] == 3);
        REQUIRE(type == inf::MeshInterface::TETRA_4);
    }
    SECTION("Collapsed 0-1 node") {
        std::vector<int> nodes = {6, 6, 7, 8};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Collapsed 1-2 node") {
        std::vector<int> nodes = {6, 7, 7, 8};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Collapsed 2-3 node") {
        std::vector<int> nodes = {6, 7, 8, 8};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Collapsed 0-2 node") {
        std::vector<int> nodes = {6, 7, 6, 8};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Collapsed 0-2 node") {
        std::vector<int> nodes = {6, 7, 8, 6};
        inf::CellCollapse::tet(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
}

TEST_CASE("Quad") {
    inf::MeshInterface::CellType type = inf::MeshInterface::QUAD_4;
    SECTION("Good Quad") {
        std::vector<int> nodes = {0, 1, 2, 3};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 0);
        REQUIRE(nodes[1] == 1);
        REQUIRE(nodes[2] == 2);
        REQUIRE(nodes[3] == 3);
        REQUIRE(type == inf::MeshInterface::QUAD_4);
    }
    SECTION("First Edge") {
        std::vector<int> nodes = {6, 6, 7, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Second Edge") {
        std::vector<int> nodes = {6, 7, 7, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Third Edge") {
        std::vector<int> nodes = {6, 7, 8, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Fourth Edge") {
        std::vector<int> nodes = {6, 7, 8, 6};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 7);
        REQUIRE(nodes[2] == 8);
        REQUIRE(nodes[3] == -1);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("Cross Collapse 1") {
        std::vector<int> nodes = {6, 7, 6, 9};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == -1);
        REQUIRE(nodes[1] == -1);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(int(type) == -1);
    }
    SECTION("Cross Collapse 1") {
        std::vector<int> nodes = {6, 7, 8, 7};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == -1);
        REQUIRE(nodes[1] == -1);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(int(type) == -1);
    }

    SECTION("First Two Edge Collapse ") {
        std::vector<int> nodes = {6, 6, 8, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 8);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(inf::MeshInterface::CellType::BAR_2 == type);
    }

    SECTION("Cross Two Edge Collapse ") {
        std::vector<int> nodes = {6, 8, 8, 6};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 8);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(inf::MeshInterface::CellType::BAR_2 == type);
    }

    SECTION("First Two Chain Collapse ") {
        std::vector<int> nodes = {6, 6, 6, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 8);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(inf::MeshInterface::CellType::BAR_2 == type);
    }

    SECTION("Last Two Chain Collapse ") {
        std::vector<int> nodes = {6, 8, 8, 8};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == 8);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(inf::MeshInterface::CellType::BAR_2 == type);
    }

    SECTION("Node Collapse ") {
        std::vector<int> nodes = {6, 6, 6, 6};
        inf::CellCollapse::quad(nodes.data(), type);
        REQUIRE(nodes[0] == 6);
        REQUIRE(nodes[1] == -1);
        REQUIRE(nodes[2] == -1);
        REQUIRE(nodes[3] == -1);
        REQUIRE(inf::MeshInterface::CellType::NODE == type);
    }
}

TEST_CASE("Triangle") {
    inf::MeshInterface::CellType type = inf::MeshInterface::TRI_3;
    SECTION("Good triangle") {
        std::vector<int> triangle = {0, 1, 2};
        inf::CellCollapse::triangle(triangle.data(), type);
        REQUIRE(triangle[0] == 0);
        REQUIRE(triangle[1] == 1);
        REQUIRE(triangle[2] == 2);
        REQUIRE(type == inf::MeshInterface::TRI_3);
    }
    SECTION("First edge collapse") {
        std::vector<int> triangle = {7, 7, 8};
        inf::CellCollapse::triangle(triangle.data(), type);
        REQUIRE(triangle[0] == 7);
        REQUIRE(triangle[1] == 8);
        REQUIRE(triangle[2] == -1);
        REQUIRE(type == inf::MeshInterface::BAR_2);
    }
    SECTION("Second edge collapse") {
        std::vector<int> triangle = {7, 8, 8};
        inf::CellCollapse::triangle(triangle.data(), type);
        REQUIRE(triangle[0] == 7);
        REQUIRE(triangle[1] == 8);
        REQUIRE(triangle[2] == -1);
        REQUIRE(type == inf::MeshInterface::BAR_2);
    }
    SECTION("Third edge collapse") {
        std::vector<int> triangle = {7, 8, 7};
        inf::CellCollapse::triangle(triangle.data(), type);
        REQUIRE(triangle[0] == 7);
        REQUIRE(triangle[1] == 8);
        REQUIRE(triangle[2] == -1);
        REQUIRE(type == inf::MeshInterface::BAR_2);
    }
    SECTION("Node collapse") {
        std::vector<int> triangle = {7, 7, 7};
        inf::CellCollapse::triangle(triangle.data(), type);
        REQUIRE(triangle[0] == 7);
        REQUIRE(triangle[1] == -1);
        REQUIRE(triangle[2] == -1);
        REQUIRE(type == inf::MeshInterface::NODE);
    }
}
