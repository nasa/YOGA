#include <RingAssertions.h>
#include <t-infinity/MeshSubdomain.h>
#include <t-infinity/CartMesh.h>
#include "MockMeshes.h"

using namespace inf;

TEST_CASE("Can color nodes in subdomain") {
    std::set<int> domain_nodes = {1, 4};
    std::vector<std::vector<int>> n2n;
    n2n.push_back({0, 2});
    n2n.push_back({1, 3});
    n2n.push_back({2, 4});
    n2n.push_back({3, 0});
    n2n.push_back({4});
    int distance = 2;
    auto colors = Subdomain::labelNodeDistance(domain_nodes, distance, n2n, [&](const auto& v) -> void {});

    REQUIRE(n2n.size() == colors.size());
    REQUIRE(2 == colors[0]);
    REQUIRE(0 == colors[1]);
    REQUIRE(Subdomain::OUTSIDE_DOMAIN == colors[2]);
    REQUIRE(1 == colors[3]);
    REQUIRE(0 == colors[4]);
}

TEST_CASE("Can build a subdomain filter from node colors") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = std::make_shared<TinfMesh>(mock::twoTouchingTets(), mp.Rank());
    std::vector<int> subdomain_nodes = {0};
    auto subdomain_filter = Subdomain::buildSubdomainFilter(mp.getCommunicator(), mesh, subdomain_nodes);
    REQUIRE(4 == subdomain_filter->getMesh()->nodeCount());
}

TEST_CASE("Can inflate subdomain") {
    std::set<int> domain_nodes = {1, 4};
    std::vector<std::vector<int>> n2n;
    n2n.push_back({0, 2});
    n2n.push_back({1, 3});
    n2n.push_back({2, 4});
    n2n.push_back({3, 0});
    n2n.push_back({4});
    auto sync = [&](const auto& v) -> void {};
    int distance = 2;
    auto colors = Subdomain::labelNodeDistance(domain_nodes, distance, n2n, sync);
    auto inflated_domain_nodes = Subdomain::addNeighborNodes(domain_nodes, distance, n2n, sync);
    REQUIRE(std::set<int>{0, 1, 3, 4} == inflated_domain_nodes);
}

TEST_CASE("Can sync a mesh -> subdomain mesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 10, 10, 10);
    Parfait::Point<double> linear_field(1.5, 2.0, 3.0);
    std::vector<double> node_values(mesh->nodeCount());
    for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
        node_values[node_id] = Parfait::Point<double>::dot(linear_field, mesh->node(node_id));
    }

    // Build subdomain with all nodes between 0.25 < x < 0.75
    std::vector<int> subdomain_nodes;
    for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
        double x = mesh->node(node_id)[0];
        if (x > 0.25 and x < 0.75) subdomain_nodes.push_back(node_id);
    }

    auto sd = MeshSubdomain(mp.getCommunicator(), mesh, subdomain_nodes);
    auto subdomain_mesh = sd.subdomain_mesh;
    std::vector<double> node_costs(sd.subdomain_mesh->nodeCount(), 1.0);
    sd.balanceSubdomainNodes(node_costs);
    sd.inflateSubdomainGhostNodes();
    auto load_balanced_subdomain_mesh = sd.subdomain_mesh;
    REQUIRE(subdomain_mesh != load_balanced_subdomain_mesh);
    REQUIRE(globalNodeCount(mp, *subdomain_mesh) == globalNodeCount(mp, *load_balanced_subdomain_mesh));

    std::vector<double> subdomain_values(load_balanced_subdomain_mesh->nodeCount(), -1.0);
    sd.syncOriginalToSubdomain(node_values.data(), subdomain_values.data(), 1);

    for (int node_id = 0; node_id < load_balanced_subdomain_mesh->nodeCount(); ++node_id) {
        if (load_balanced_subdomain_mesh->ownedNode(node_id)) {
            auto expected = Parfait::Point<double>::dot(linear_field, load_balanced_subdomain_mesh->node(node_id));
            REQUIRE(expected == subdomain_values.at(node_id));
        }
    }

    std::vector<double> original_values_from_subdomain(mesh->nodeCount(), -1.0);
    sd.syncSubdomainToOriginal(subdomain_values.data(), original_values_from_subdomain.data(), 1);
    for (int node_id : subdomain_nodes) {
        auto expected = Parfait::Point<double>::dot(linear_field, mesh->node(node_id));
        REQUIRE(expected == original_values_from_subdomain[node_id]);
    }
}
