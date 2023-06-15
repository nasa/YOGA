#include "distance_calculator.h"
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/PancakeMeshAdapterFrom.h>
#include <tinf_mesh.h>
#include <t-infinity/DistanceCalculator.h>
#include <memory>

class Fun3dDistanceCalculator {
  public:
    Fun3dDistanceCalculator(MPI_Comm comm, std::shared_ptr<inf::MeshInterface> mesh) : mp(comm), mesh(mesh) {}

    void calculate(std::set<int> tags) {
        int max_depth = 10;
        int max_objects_per_voxel = 20;
        points_and_metadata = inf::calculateDistanceAndMetaData(mp, *mesh, tags, max_depth, max_objects_per_voxel);
    }

  public:
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> mesh;
    std::vector<std::tuple<Parfait::Point<double>, inf::Distance::MetaData>> points_and_metadata;
};

int32_t calculate_distance_to_nodes(
    int fortran_comm, void* tinf_mesh, int64_t* wall_tags, int64_t num_tags, double* node_distance) {
    try {
        auto comm = MPI_Comm_f2c(fortran_comm);
        auto mp = MessagePasser(comm);
        auto mesh = std::make_shared<inf::PancakeMeshAdapterFrom>(tinf_mesh);
        std::set<int> tags(wall_tags, wall_tags + num_tags);

        // auto points_and_metadata = inf::calculateDistanceAndMetaData(mp, *mesh, tags);
        auto dist = inf::calculateDistance(mp, *mesh, tags);
        std::copy(dist.begin(), dist.end(), node_distance);
        return TINF_SUCCESS;
    } catch (std::exception& e) {
        printf("distance calculator failed with error %s\n", e.what());
        return TINF_FAILURE;
    }
}

int32_t mangle(tinf_distance_create)(void** handle, int fortran_comm, void* tinf_mesh) {
    auto comm = MPI_Comm_f2c(fortran_comm);
    auto mesh = std::make_shared<inf::PancakeMeshAdapterFrom>(tinf_mesh);
    auto c = new Fun3dDistanceCalculator(comm, mesh);
    if (c == nullptr) {
        return TINF_FAILURE;
    } else {
        *handle = c;
        return TINF_SUCCESS;
    }
}

int32_t mangle(tinf_distance_destroy)(void** handle) {
    auto c = (Fun3dDistanceCalculator*)*handle;
    delete c;
    *handle = nullptr;
    return TINF_SUCCESS;
}

int32_t mangle(tinf_distance_calculate)(void* handle, int64_t* wall_tags, int64_t num_tags) {
    auto c = (Fun3dDistanceCalculator*)handle;
    if (c == nullptr) {
        return TINF_FAILURE;
    }
    std::set<int> wall_tags_set;
    for (int64_t i = 0; i < num_tags; i++) {
        wall_tags_set.insert(wall_tags[i]);
    }

    c->calculate(wall_tags_set);

    return TINF_SUCCESS;
}

int32_t mangle(tinf_distance_point_on_surface)(void* self, double* x, double* y, double* z) {
    auto c = (Fun3dDistanceCalculator*)self;
    if (c == nullptr) {
        return TINF_FAILURE;
    }

    for (int n = 0; n < c->mesh->nodeCount(); n++) {
        Parfait::Point<double> p;
        inf::Distance::MetaData m;
        std::tie(p, m) = c->points_and_metadata[n];
        x[n] = p[0];
        y[n] = p[1];
        z[n] = p[2];
    }

    return TINF_SUCCESS;
}

int32_t mangle(tinf_distance_distance_to_surface)(void* self, double* distance) {
    auto c = (Fun3dDistanceCalculator*)self;
    if (c == nullptr) {
        return TINF_FAILURE;
    }
    for (int n = 0; n < c->mesh->nodeCount(); n++) {
        Parfait::Point<double> p;
        inf::Distance::MetaData m;
        std::tie(p, m) = c->points_and_metadata[n];
        Parfait::Point<double> q = c->mesh->node(n);
        distance[n] = Parfait::Point<double>::distance(p, q);
    }

    return TINF_SUCCESS;
}

int32_t mangle(tinf_distance_global_cell_id_of_surface_element)(void* self, int64_t* gcid) {
    auto c = (Fun3dDistanceCalculator*)self;
    if (c == nullptr) {
        return TINF_FAILURE;
    }
    for (int n = 0; n < c->mesh->nodeCount(); n++) {
        Parfait::Point<double> p;
        inf::Distance::MetaData m;
        std::tie(p, m) = c->points_and_metadata[n];
        gcid[n] = m.gcid;
    }

    return TINF_SUCCESS;
}

int32_t mangle(tinf_distance_cell_owner_of_surface_cell)(void* self, int64_t* owner) {
    auto c = (Fun3dDistanceCalculator*)self;
    if (c == nullptr) {
        return TINF_FAILURE;
    }
    for (int n = 0; n < c->mesh->nodeCount(); n++) {
        Parfait::Point<double> p;
        inf::Distance::MetaData m;
        std::tie(p, m) = c->points_and_metadata[n];
        owner[n] = m.owner;
    }

    return TINF_SUCCESS;
}
