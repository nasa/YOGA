#include "MeshSubdomain.h"
#include "Extract.h"
#include "MeshShuffle.h"
#include "AddHalo.h"

std::vector<long> invert(const std::map<long, int>& m) {
    std::vector<long> v(m.size());
    for (const auto& p : m) v[p.second] = p.first;
    return v;
}
std::map<long, int> invert(const std::vector<long>& v) {
    std::map<long, int> m;
    for (int i = 0; i < (int)v.size(); ++i) {
        m[v.at(i)] = i;
    }
    return m;
}

inf::MeshSubdomain::MeshSubdomain(MPI_Comm comm,
                                  std::shared_ptr<MeshInterface> mesh,
                                  const std::vector<int>& subdomain_nodes)
    : mp(comm), subdomain_nodes_on_original_mesh(subdomain_nodes), original_mesh(mesh) {
    auto subdomain_filter = Subdomain::buildSubdomainFilter(comm, mesh, subdomain_nodes);
    subdomain_mesh = subdomain_filter->getMesh();
    subdomain_g2l = invert(
        FieldShuffle::buildLocalToPreviousGlobalNode(*original_mesh, *subdomain_filter->getMesh()));
    original_g2l = GlobalToLocal::buildNode(*original_mesh);
}
Parfait::SyncPattern inf::MeshSubdomain::buildOriginalToSubdomainSyncPattern() const {
    return {mp, extractOwnedGlobalNodeIds(*original_mesh), invert(subdomain_g2l)};
}
Parfait::SyncPattern inf::MeshSubdomain::buildSubdomainToOriginalSyncPattern() const {
    std::vector<long> need;
    for (int node_id : subdomain_nodes_on_original_mesh)
        need.push_back(original_mesh->globalNodeId(node_id));
    return {mp, invert(subdomain_g2l), need};
}
void inf::MeshSubdomain::balanceSubdomainNodes(const std::vector<double>& node_costs) {
    auto load_balanced_part_vector = MeshShuffle::repartitionNodes(mp, *subdomain_mesh, node_costs);
    auto load_balanced_subdomain_mesh =
        MeshShuffle::shuffleNodes(mp, *subdomain_mesh, load_balanced_part_vector);
    auto subdomain_local_to_original_global_node_ids = FieldShuffle::shuffleNodeField(
        mp, invert(subdomain_g2l), *subdomain_mesh, *load_balanced_subdomain_mesh);
    subdomain_g2l = invert(subdomain_local_to_original_global_node_ids);
    subdomain_mesh = load_balanced_subdomain_mesh;
}
void inf::MeshSubdomain::inflateSubdomainGhostNodes() {
    auto l2g_before = LocalToGlobal::buildNode(*subdomain_mesh);
    subdomain_mesh = addHaloNodes(mp, *subdomain_mesh);
    auto g2l_after = GlobalToLocal::buildNode(*subdomain_mesh);
    for (auto& p : subdomain_g2l) {
        int previous_gid = l2g_before.at(p.second);
        int inflated_mesh_node_id = g2l_after.at(previous_gid);
        p.second = inflated_mesh_node_id;
    }
}
