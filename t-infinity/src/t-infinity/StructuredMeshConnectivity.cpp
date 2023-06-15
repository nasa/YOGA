#include <parfait/Throw.h>
#include "StructuredMeshConnectivity.h"
#include "StructuredBlockConnectivity.h"
#include <parfait/FileTools.h>
#include <parfait/StringTools.h>
#include <Tracer.h>

struct BlockToFaces {
    int block_id;
    std::array<inf::BlockFace, 6> faces;
};

inf::MeshConnectivity inf::buildMeshBlockConnectivity(const MessagePasser& mp,
                                                      const inf::StructuredMesh& mesh) {
    TRACER_PROFILE_FUNCTION
    std::map<int, std::vector<BlockToFaces>> all_sends;
    int dest_rank = 0;

    for (int block_id : mesh.blockIds()) {
        const auto& block = mesh.getBlock(block_id);
        std::array<BlockFace, 6> faces{};
        for (auto side : AllBlockSides) faces[side] = getBlockFaces(block, side);
        all_sends[dest_rank].push_back({block_id, faces});
    }

    auto all_messages = mp.Exchange(all_sends);

    AllBlockFaces all_faces;
    if (mp.Rank() == 0) {
        for (const auto& p : all_messages) {
            for (const auto& block_to_faces : p.second) {
                int block_id = block_to_faces.block_id;
                all_faces[block_id] = block_to_faces.faces;
            }
        }
    }

    mp.Broadcast(all_faces, 0);

    MeshConnectivity mesh_connectivity;
    for (int block_id : mesh.blockIds()) {
        mesh_connectivity[block_id] = {};  // initialize connectivity
        const auto& block_faces = all_faces.at(block_id);
        for (const auto& face : block_faces) {
            matchFace(mesh_connectivity, all_faces, face);
        }
    }
    return mesh_connectivity;
}

void inf::matchFace(inf::MeshConnectivity& mesh_connectivity,
                    const inf::AllBlockFaces& all_faces,
                    const inf::BlockFace& face) {
    for (const auto& p : all_faces) {
        if (p.first == face.block_id) continue;
        for (const auto& neighbor : p.second) {
            if (blockFacesMatch(face.corners, neighbor.corners)) {
                mesh_connectivity[face.block_id][face.side] = getFaceConnectivity(face, neighbor);
                return;
            }
        }
    }
}

enum OppositeFaceFirstIndex {
    FirstIndexSameDirection = 1,
    FirstIndexOppositeDirection = 2,
    SecondIndexSameDirection = 3,
    SecondIndexOppositeDirection = 4
};

struct OppositeBlockFace {
    OppositeBlockFace() = default;
    explicit OppositeBlockFace(int bc) {
        int base = std::pow(10, int(log10(bc)));
        block_id = (bc - base) / 1000 - 1;
        side = bc / 100 % 10 - 1;
        direction1 = bc / 10 % 10;
        direction2 = bc % 10;
    }
    int block_id;
    int side;
    int direction1;
    int direction2;
};

inf::BlockFaceConnectivity convertConnectivity(
    const std::map<inf::BlockSide, OppositeBlockFace>& laura_connectivity) {
    using namespace inf;
    std::map<BlockSide, FaceConnectivity> inf_connectivity;
    for (const auto& face : laura_connectivity) {
        auto side = static_cast<BlockSide>(face.first);
        auto& neighbor = inf_connectivity[side];
        neighbor.neighbor_block_id = face.second.block_id;
        neighbor.neighbor_side = static_cast<BlockSide>(face.second.side);

        BlockAxis host_primary_axis = sideToAxis(side);
        BlockAxis neighbor_primary_axis = sideToAxis(neighbor.neighbor_side);
        neighbor.face_mapping[host_primary_axis].axis = neighbor_primary_axis;
        neighbor.face_mapping[host_primary_axis].direction =
            (isMaxFace(side) xor isMaxFace(neighbor.neighbor_side)) ? inf::SameDirection
                                                                    : inf::ReversedDirection;

        auto host_first_tangent_axis = TangentAxis::sideToFirst(side);
        auto host_second_tangent_axis = TangentAxis::sideToSecond(side);
        auto neib_first_tangent_axis = TangentAxis::sideToFirst(neighbor.neighbor_side);
        auto neib_second_tangent_axis = TangentAxis::sideToSecond(neighbor.neighbor_side);

        auto& first_tangent_axis = neighbor.face_mapping[host_first_tangent_axis];
        auto& second_tangent_axis = neighbor.face_mapping[host_second_tangent_axis];
        switch (face.second.direction1) {
            case FirstIndexSameDirection: {
                first_tangent_axis.axis = neib_first_tangent_axis;
                first_tangent_axis.direction = inf::SameDirection;
                second_tangent_axis.axis = neib_second_tangent_axis;
                break;
            }
            case FirstIndexOppositeDirection: {
                first_tangent_axis.axis = neib_first_tangent_axis;
                first_tangent_axis.direction = inf::ReversedDirection;
                second_tangent_axis.axis = neib_second_tangent_axis;
                break;
            }
            case SecondIndexSameDirection: {
                first_tangent_axis.axis = neib_second_tangent_axis;
                first_tangent_axis.direction = inf::SameDirection;
                second_tangent_axis.axis = neib_first_tangent_axis;
                break;
            }
            case SecondIndexOppositeDirection: {
                first_tangent_axis.axis = neib_second_tangent_axis;
                first_tangent_axis.direction = inf::ReversedDirection;
                second_tangent_axis.axis = neib_first_tangent_axis;
                break;
            }
            default:
                PARFAIT_THROW("Unknown block face orientation: " +
                              std::to_string(face.second.direction1));
        }
        second_tangent_axis.direction =
            face.second.direction2 == 1 ? SameDirection : ReversedDirection;
    }
    return inf_connectivity;
}

inf::BlockFaceConnectivity inf::buildBlockConnectivityFromLAURABoundaryConditions(
    const std::array<int, 6>& boundary_conditions) {
    std::map<BlockSide, OppositeBlockFace> mapping;
    for (auto side : AllBlockSides) {
        auto bc = boundary_conditions[side];
        if (bc > 1000000) mapping[side] = OppositeBlockFace(bc);
    }
    return convertConnectivity(mapping);
}
inf::MeshConnectivity inf::buildMeshConnectivityFromLAURABoundaryConditions(
    const std::string& filename) {
    auto contents = Parfait::FileTools::loadFileToString(filename);
    MeshConnectivity connectivity;
    int block_id = 0;
    for (const auto& line : Parfait::StringTools::split(contents, "\n")) {
        auto split_line = Parfait::StringTools::split(line, " ");
        std::array<int, 6> boundary_conditions;
        for (int side = 0; side < 6; ++side)
            boundary_conditions[side] = Parfait::StringTools::toInt(split_line[side]);
        connectivity[block_id++] =
            buildBlockConnectivityFromLAURABoundaryConditions(boundary_conditions);
    }
    return connectivity;
}
