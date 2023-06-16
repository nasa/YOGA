#pragma once

#include <utility>
#include "StructuredBlockPlane.h"
#include "StructuredMeshConnectivity.h"
#include "TopologicalRotation.h"
#include "StructuredMeshHelpers.h"
#include <parfait/MapTools.h>
#include <parfait/Throw.h>
#include <Tracer.h>

namespace inf {
class StructuredBlockPlaneSyncer {
  public:
    enum Type { Node = 0, Cell = 1 };
    StructuredBlockPlaneSyncer(const MessagePasser& mp,
                               MeshConnectivity mesh_connectivity,
                               std::map<int, std::array<int, 3>> block_dimensions,
                               int overlap,
                               Type sync_type)
        : mp(mp),
          mesh_connectivity(std::move(mesh_connectivity)),
          block_dimensions(std::move(block_dimensions)),
          block_to_rank(buildBlockToRank(mp, getBlocksOnRank())),
          block_overlap(overlap),
          sync_type(sync_type) {}

    template <typename GetValue, typename SetValue>
    void sync(const GetValue& get_value, const SetValue& set_value, int ghost_layer) const {
        TRACER_PROFILE_FUNCTION
        PARFAIT_ASSERT(ghost_layer <= block_overlap,
                       "requested ghost layer greater than block overlap");
        using value_type = decltype(get_value(0, 0, 0, 0));

        int source_offset = block_overlap + ghost_layer;

        std::vector<int> expected_recv_ranks;
        std::map<int, MessagePasser::Message> all_messages;
        for (const auto& p : mesh_connectivity) {
            int block_id = p.first;
            auto get_value_on_block = [&](int i, int j, int k) {
                return get_value(block_id, i, j, k);
            };
            for (const auto& face : p.second) {
                auto side = face.first;
                const auto& connectivity = face.second;
                const auto& dimensions = block_dimensions.at(block_id);
                auto axis = sideToAxis(side);
                int axis_largest_index = dimensions[axis] - 1;
                int slice_index =
                    isMaxFace(side) ? axis_largest_index - source_offset : source_offset;
                StructuredBlockPlane<value_type> slice(
                    get_value_on_block, dimensions, axis, slice_index);

                int destination_rank = block_to_rank.at(connectivity.neighbor_block_id);
                auto& msg = all_messages[destination_rank];
                msg.pack(connectivity.neighbor_block_id);
                msg.pack(block_id);
                msg.pack(connectivity.neighbor_side);
                msg.pack(slice.original_block_dimensions);
                msg.pack(slice.slice_axis);
                msg.pack(slice.slice_index);
                msg.pack(slice.plane.vec);

                expected_recv_ranks.push_back(destination_rank);
            }
        }

        auto exchanged_planes = mp.Exchange(all_messages);

        int target_offset =
            block_overlap - ghost_layer - sync_type;  // Shift for cells, but not nodes

        for (int rank : expected_recv_ranks) {
            auto& msg = exchanged_planes[rank];

            int host_block_id = -1, neighbor_block_id = -1;
            BlockSide host_side =
                BlockSide(-1);  // set to something invalid because of compiler warnings.
            msg.unpack(host_block_id);
            msg.unpack(neighbor_block_id);
            msg.unpack(host_side);

            StructuredBlockPlane<value_type> slice;
            msg.unpack(slice.original_block_dimensions);
            msg.unpack(slice.slice_axis);
            msg.unpack(slice.slice_index);
            slice.plane = MDVector<value_type, 2>(slice.getPlaneDimensions());
            msg.unpack(slice.plane.vec);

            const auto& host_to_neighbor_axis_mapping =
                mesh_connectivity.at(host_block_id).at(host_side).face_mapping;
            TopologicalRotation rotation(host_to_neighbor_axis_mapping,
                                         slice.original_block_dimensions);

            std::array<int, 3> start = {0, 0, 0};
            std::array<int, 3> end = block_dimensions.at(host_block_id);

            auto host_axis = sideToAxis(host_side);
            int axis_largest_index = end[host_axis] - 1;
            start[host_axis] =
                isMaxFace(host_side) ? axis_largest_index - target_offset : target_offset;
            end[host_axis] = start[host_axis] + 1;
            loopMDRange(start, end, [&](int i, int j, int k) {
                auto neighbor_ijk = rotation.convertHostToNeighbor(i, j, k);
                neighbor_ijk[slice.slice_axis] = slice.slice_index;
                set_value(host_block_id,
                          i,
                          j,
                          k,
                          slice(neighbor_ijk[0], neighbor_ijk[1], neighbor_ijk[2]));
            });
        }
    }

    MessagePasser mp;
    MeshConnectivity mesh_connectivity;
    std::map<int, std::array<int, 3>> block_dimensions;
    std::map<int, int> block_to_rank;
    int block_overlap;
    Type sync_type;

    std::vector<int> getBlocksOnRank() const {
        auto blocks = Parfait::MapTools::keys(mesh_connectivity);
        return {blocks.begin(), blocks.end()};
    }
};
}
