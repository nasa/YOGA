#pragma once
#include <parfait/SyncField.h>
#include "MeshInterface.h"
#include "CellSelectedMesh.h"
#include "GlobalToLocal.h"
#include "VectorFieldAdapter.h"
#include "MeshHelpers.h"
#include <parfait/Checkpoint.h>

namespace inf {
namespace FieldShuffle {

    std::vector<long> buildLocalToPreviousGlobalNode(const MeshInterface& source,
                                                     const CellSelectedMesh& target);

    Parfait::SyncPattern buildMeshToMeshNodeSyncPattern(const MessagePasser& mp,
                                                        const MeshInterface& source,
                                                        const MeshInterface& target);
    template <typename FieldType>
    Parfait::StridedFieldSyncer<double, FieldType> buildMeshToMeshFieldSyncer(
        const MessagePasser& mp, FieldType& field, const Parfait::SyncPattern& sync_pattern) {
        return Parfait::StridedFieldSyncer<double, FieldType>(mp, field, sync_pattern);
    }

    template <typename T, typename MapType>
    Parfait::StridedSyncer<T, MapType> buildMeshToMeshFieldStridedSyncer(
        const MessagePasser& mp,
        T* source,
        T* target,
        const Parfait::SyncPattern& sync_pattern,
        const MapType& source_g2l,
        const MapType& target_g2l,
        int stride) {
        return Parfait::StridedSyncer<T, MapType>(
            mp, source, target, sync_pattern, source_g2l, target_g2l, stride);
    }

    template <typename T>
    struct FieldMeshToMeshWrapper {
        FieldMeshToMeshWrapper(const std::vector<T>& source_values,
                               std::vector<T>& target_values,
                               std::map<long, int> source_g2l,
                               std::map<long, int> target_g2l,
                               int stride)
            : source_mesh(source_values),
              target_mesh(target_values),
              source_g2l(std::move(source_g2l)),
              target_g2l(std::move(target_g2l)),
              block_size(stride) {}
        double getEntry(long gid, int i) const {
            return source_mesh[source_g2l.at(gid) * block_size + i];
        }
        void setEntry(long gid, int i, const T& value) {
            target_mesh[target_g2l.at(gid) * block_size + i] = value;
        }
        int stride() const { return block_size; }

        const std::vector<T>& source_mesh;
        std::vector<T>& target_mesh;
        const std::map<long, int> source_g2l;
        const std::map<long, int> target_g2l;
        const int block_size;
    };

    template <typename T>
    std::vector<T> shuffleNodeField(const MessagePasser& mp,
                                    const std::vector<T>& source_field,
                                    const MeshInterface& source,
                                    const MeshInterface& target,
                                    int stride = 1) {
        std::vector<T> target_field(target.nodeCount() * stride);
        auto sync_pattern = buildMeshToMeshNodeSyncPattern(mp, source, target);
        auto f = FieldMeshToMeshWrapper<T>(source_field,
                                           target_field,
                                           GlobalToLocal::buildNode(source),
                                           GlobalToLocal::buildNode(target),
                                           stride);
        auto syncer = buildMeshToMeshFieldSyncer(mp, f, sync_pattern);
        syncer.start();
        syncer.finish();
        return target_field;
    }
    template <typename T>
    std::shared_ptr<FieldInterface> shuffleNodeField(const MessagePasser& mp,
                                                     const MeshInterface& source,
                                                     const MeshInterface& target,
                                                     const FieldInterface& source_field) {
        PARFAIT_ASSERT(source_field.association() == FieldAttributes::Node(),
                       "expected Node association");
        PARFAIT_ASSERT(source_field.size() == source.nodeCount(), "field/mesh size mismatch");

        std::vector<T> source_values(source_field.size() * source_field.blockSize());
        for (int node_id = 0; node_id < source.nodeCount(); ++node_id) {
            source_field.value(node_id, &source_values[node_id * source_field.blockSize()]);
        }
        auto target_values =
            shuffleNodeField(mp, source_values, source, target, source_field.blockSize());
        return std::make_shared<VectorFieldAdapter>(
            source_field.name(), FieldAttributes::Node(), source_field.blockSize(), target_values);
    }
}
}