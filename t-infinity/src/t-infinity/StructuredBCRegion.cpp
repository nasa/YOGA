#include "StructuredBCRegion.h"
#include <parfait/StringTools.h>
#include <t-infinity/FaceNeighbors.h>
#include "MDVector.h"
#include "StructuredToUnstructuredMeshAdapter.h"

inf::BlockAxis inf::StructuredBCRegion::axisFromString(std::string IorJorK) {
    IorJorK = Parfait::StringTools::strip(Parfait::StringTools::toupper(IorJorK));
    if (IorJorK == "I") return BlockAxis::I;
    if (IorJorK == "J") return BlockAxis::J;
    if (IorJorK == "K") return BlockAxis::K;
    PARFAIT_THROW("String <" + IorJorK + "> is not an I,J,K axis");
}
int inf::StructuredBCRegion::rangeIntFromString(std::string s) {
    if (Parfait::StringTools::isInteger(s)) return Parfait::StringTools::toInt(s) - 1;
    s = Parfait::StringTools::toupper(s);
    if (s == "MIN") return MIN;
    if (s == "MAX") return MAX;
    PARFAIT_THROW("Could not parse range integer: " + s);
}
inf::BlockSide inf::StructuredBCRegion::sideFromString(std::string side_string) {
    side_string = Parfait::StringTools::toupper(side_string);
    if (side_string == "IMIN") return IMIN;
    if (side_string == "IMAX") return IMAX;
    if (side_string == "JMIN") return JMIN;
    if (side_string == "JMAX") return JMAX;
    if (side_string == "KMIN") return KMIN;
    if (side_string == "KMAX") return KMAX;
    PARFAIT_THROW("Could not parse block side from string: " + side_string);
}
std::vector<inf::StructuredBCRegion> inf::StructuredBCRegion::parse(
    std::vector<std::string> lines) {
    std::vector<StructuredBCRegion> patches;

    for (auto line : lines) {
        // skip comment lines
        line = Parfait::StringTools::strip(line);
        if (Parfait::StringTools::startsWith(line, "#")) continue;
        patches.push_back(parse(line));
    }

    return patches;
}
std::string splitSideToString(inf::BlockSide side) {
    if (side == inf::IMIN) return "I MIN";
    if (side == inf::IMAX) return "I MAX";
    if (side == inf::JMIN) return "J MIN";
    if (side == inf::JMAX) return "J MAX";
    if (side == inf::KMIN) return "K MIN";
    if (side == inf::KMAX) return "K MAX";
    PARFAIT_THROW("Invalid enum, should never occur");
}

std::string axisToString(inf::BlockAxis axis) {
    if (axis == inf::I) return "I";
    if (axis == inf::J) return "J";
    if (axis == inf::K) return "K";
    PARFAIT_THROW("Invalid enum, should never occur");
}

std::string rangeIntToString(int i) {
    if (i == inf::StructuredBCRegion::MIN) return "MIN";
    if (i == inf::StructuredBCRegion::MAX) return "MAX";
    return std::to_string(i + 1);
}

std::string inf::StructuredBCRegion::to_string() const {
    std::string s;
    s = name + " ";
    s += std::to_string(host_block + 1) + " ";
    s += splitSideToString(side) + " ";
    s += axisToString(axis1) + " ";
    s += rangeIntToString(axis1_range[0]) + " " + rangeIntToString(axis1_range[1]) + " ";
    s += axisToString(axis2) + " ";
    s += rangeIntToString(axis2_range[0]) + " " + rangeIntToString(axis2_range[1]) + " ";
    return s;
}

inf::StructuredBCRegion inf::StructuredBCRegion::parse(std::string line) {
    using namespace Parfait::StringTools;
    // Parse using Vulcan's syntax.
    StructuredBCRegion patch;
    auto words = split(line, " ");
    PARFAIT_ASSERT(words.size() >= 10, "Vulcan BC patch strings must have at least 10 words");
    patch.name = words[0];
    patch.host_block = toInt(words[1]) - 1;
    //                          I J K      MIN MAX
    patch.side = sideFromString(words[2] + words[3]);

    patch.axis1 = axisFromString(words[4]);
    patch.axis1_range[0] = rangeIntFromString(words[5]);
    patch.axis1_range[1] = rangeIntFromString(words[6]);
    patch.axis2 = axisFromString(words[7]);
    patch.axis2_range[0] = rangeIntFromString(words[8]);
    patch.axis2_range[1] = rangeIntFromString(words[9]);
    return patch;
}
void inf::StructuredBCRegion::reassignBoundaryTagsFromBCRegions(
    inf::StructuredTinfMesh& mesh, const std::vector<StructuredBCRegion>& bc_regions) {
    auto face_neighbors = inf::FaceNeighbors::build(mesh);

    int region_number = 0;
    for (auto& region : bc_regions) {
        printf("Processing region %s\n", region.to_string().c_str());
        int face_neighbor_index = BlockSideToHexOrdering[region.side];
        auto block_id = region.host_block;
        auto& block = mesh.getBlock(block_id);
        auto start = region.start(block.nodeDimensions());
        auto end = region.end(block.nodeDimensions());
        inf::loopMDRange(start, end, [&](int i, int j, int k) {
            if (i < 0 or i >= block.cellDimensions()[0]) {
                PARFAIT_THROW("i " + std::to_string(i) + " is out of range ");
            }
            if (j < 0 or j >= block.cellDimensions()[1]) {
                PARFAIT_THROW("j " + std::to_string(j) + " is out of range ");
            }
            if (k < 0 or k >= block.cellDimensions()[2]) {
                PARFAIT_THROW("k " + std::to_string(k) + " is out of range ");
            }
            auto cell_id = mesh.cellId(block_id, i, j, k);
            auto neighbor_surface_cell = face_neighbors[cell_id][face_neighbor_index];
            PARFAIT_ASSERT(
                neighbor_surface_cell >= 0,
                "Invalid neighbor surface cell " + std::to_string(neighbor_surface_cell));
            auto pair = mesh.cell_id_to_type_and_local_id[neighbor_surface_cell];
            mesh.mesh.cell_tags[pair.first][pair.second] = region_number;
        });
        mesh.setTagName(region_number, region.name);
        region_number++;
    }
}

bool isMin(inf::BlockSide side) {
    using namespace inf;
    if (side == IMIN or side == JMIN or side == KMIN) return true;
    return false;
}

std::array<int, 3> inf::StructuredBCRegion::start(const std::array<int, 3>& node_dimensions) const {
    auto face_axis = getFaceAxis();
    std::array<int, 3> out;
    int face_location = isMin(side) ? 0 : node_dimensions[face_axis] - 2;
    out[face_axis] = face_location;
    out[axis1] = axis1_range[0] == MIN ? 0 : axis1_range[0];
    out[axis2] = axis2_range[0] == MIN ? 0 : axis2_range[0];
    return out;
}
std::array<int, 3> inf::StructuredBCRegion::end(const std::array<int, 3>& node_dimensions) const {
    auto face_axis = getFaceAxis();
    std::array<int, 3> out;
    int face_location = isMin(side) ? 1 : node_dimensions[face_axis] - 1;
    out[face_axis] = face_location;
    out[axis1] = axis1_range[1] == MAX ? node_dimensions[axis1] - 1 : axis1_range[1];
    out[axis2] = axis2_range[1] == MAX ? node_dimensions[axis2] - 1 : axis2_range[1];
    return out;
}
inf::BlockAxis inf::StructuredBCRegion::getFaceAxis() const {
    if (side == IMIN or side == IMAX) return I;
    if (side == JMIN or side == JMAX) return J;
    if (side == KMIN or side == KMAX) return K;
    PARFAIT_THROW("Error invalid axis detected");
}
