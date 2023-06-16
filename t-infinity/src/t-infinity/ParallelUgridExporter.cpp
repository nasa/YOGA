#include "ParallelUgridExporter.h"
#include "MeshHelpers.h"
#include "InfinityToAFLR.h"
#include <parfait/MapbcWriter.h>
#include "Extract.h"

long inf::ParallelUgridExporter::globalCellCount(inf::MeshInterface::CellType type,
                                                 const inf::MeshInterface& mesh,
                                                 const MessagePasser& mp) {
    long count = 0;
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.cellType(i) == type and mesh.cellOwner(i) == mp.Rank()) {
            count++;
        }
    }
    return mp.ParallelSum(count);
}
void inf::ParallelUgridExporter::streamCellsToFile(std::string filename,
                                                   inf::MeshInterface::CellType type,
                                                   const inf::MeshInterface& mesh,
                                                   bool swap_bytes,
                                                   const MessagePasser& mp) {
    std::vector<int> cells;
    std::vector<int> cell(mesh.cellTypeLength(type));
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.cellType(i) == type and mp.Rank() == mesh.cellOwner(i)) {
            mesh.cell(i, cell.data());
            inf::toAFLR::rewindCell(type, cell.data());
            for (int id : cell) cells.push_back(mesh.globalNodeId(id));
        }
    }
    std::transform(cells.begin(), cells.end(), cells.begin(), [](int a) { return a + 1; });
    for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
        if (mp.Rank() == rank) {
            Parfait::UgridWriter::writeIntegerField(
                filename, cells.size(), cells.data(), swap_bytes);
        }
        mp.Barrier();
    }
}
void inf::ParallelUgridExporter::streamCellTagsToFile(std::string filename,
                                                      inf::MeshInterface::CellType type,
                                                      const inf::MeshInterface& mesh,
                                                      bool swap_bytes,
                                                      const MessagePasser& mp) {
    std::vector<int> tags;
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.cellType(i) == type and mp.Rank() == mesh.cellOwner(i)) {
            tags.push_back(mesh.cellTag(i));
        }
    }
    for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
        if (mp.Rank() == rank) {
            Parfait::UgridWriter::writeIntegerField(filename, tags.size(), tags.data(), swap_bytes);
        }
        mp.Barrier();
    }
}
std::vector<long> inf::ParallelUgridExporter::extractGlobalNodeIds(int rank,
                                                                   const inf::MeshInterface& mesh,
                                                                   long start,
                                                                   long end) {
    std::vector<long> gids;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        long gid = mesh.globalNodeId(i);
        if (mesh.nodeOwner(i) == rank and gid >= start and gid < end) {
            gids.push_back(gid);
        }
    }
    return gids;
}
std::vector<Parfait::Point<double>> inf::ParallelUgridExporter::extractPoints(
    int rank, const inf::MeshInterface& mesh, long start, long end) {
    std::vector<Parfait::Point<double>> points;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        long gid = mesh.globalNodeId(i);
        if (mesh.nodeOwner(i) == rank and gid >= start and gid < end) {
            auto p = mesh.node(i);
            points.push_back(p);
        }
    }
    return points;
}
std::vector<Parfait::Point<double>> inf::ParallelUgridExporter::shuttleNodes(
    const MessagePasser& mp,
    int target_rank,
    const inf::MeshInterface& mesh,
    long start,
    long end) {
    auto gids = extractGlobalNodeIds(mp.Rank(), mesh, start, end);
    auto points = extractPoints(mp.Rank(), mesh, start, end);

    return mp.GatherByIds(points, gids, target_rank);
}
void inf::ParallelUgridExporter::write(std::string name,
                                       const inf::MeshInterface& mesh,
                                       const MessagePasser& mp,
                                       bool write_mapbc_file) {
    long total_nodes = inf::globalNodeCount(mp, mesh);
    long total_triangles = globalCellCount(MeshInterface::TRI_3, mesh, mp);
    long total_quads = globalCellCount(MeshInterface::QUAD_4, mesh, mp);
    long total_tets = globalCellCount(MeshInterface::TETRA_4, mesh, mp);
    long total_pyramids = globalCellCount(MeshInterface::PYRA_5, mesh, mp);
    long total_prisms = globalCellCount(MeshInterface::PENTA_6, mesh, mp);
    long total_hexs = globalCellCount(MeshInterface::HEXA_8, mesh, mp);

    auto filename = setUgridExtension(name);
    bool swap_bytes = hasBigEndianExtension(filename);
    mp_rootprint("Writing mesh %s\n", name.c_str());
    mp_rootprint("Nodes:     %s\n", Parfait::bigNumberToStringWithCommas(total_nodes).c_str());
    mp_rootprint("Tris:      %s\n", Parfait::bigNumberToStringWithCommas(total_triangles).c_str());
    mp_rootprint("Quads:     %s\n", Parfait::bigNumberToStringWithCommas(total_quads).c_str());
    mp_rootprint("Tets:      %s\n", Parfait::bigNumberToStringWithCommas(total_tets).c_str());
    mp_rootprint("Pyramids:  %s\n", Parfait::bigNumberToStringWithCommas(total_pyramids).c_str());
    mp_rootprint("Prisms:    %s\n", Parfait::bigNumberToStringWithCommas(total_prisms).c_str());
    mp_rootprint("Hexs:      %s\n", Parfait::bigNumberToStringWithCommas(total_hexs).c_str());
    if (mp.Rank() == 0) {
        Parfait::UgridWriter::writeHeader(filename,
                                          total_nodes,
                                          total_triangles,
                                          total_quads,
                                          total_tets,
                                          total_pyramids,
                                          total_prisms,
                                          total_hexs,
                                          swap_bytes);
    }
    auto points = shuttleNodes(mp, 0, mesh, 0, total_nodes);
    if (mp.Rank() == 0) {
        Parfait::UgridWriter::writeNodes(
            filename, points.size(), points.data()->data(), swap_bytes);
    }
    streamCellsToFile(filename, MeshInterface::TRI_3, mesh, swap_bytes, mp);
    streamCellsToFile(filename, MeshInterface::QUAD_4, mesh, swap_bytes, mp);
    streamCellTagsToFile(filename, MeshInterface::TRI_3, mesh, swap_bytes, mp);
    streamCellTagsToFile(filename, MeshInterface::QUAD_4, mesh, swap_bytes, mp);
    streamCellsToFile(filename, MeshInterface::TETRA_4, mesh, swap_bytes, mp);
    streamCellsToFile(filename, MeshInterface::PYRA_5, mesh, swap_bytes, mp);
    streamCellsToFile(filename, MeshInterface::PENTA_6, mesh, swap_bytes, mp);
    streamCellsToFile(filename, MeshInterface::HEXA_8, mesh, swap_bytes, mp);

    Parfait::BoundaryConditionMap bc_map;
    auto tags = extractAll2DTags(mp, mesh);
    for (const auto& t : tags) {
        bc_map[t] = std::make_pair(t, mesh.tagName(t));
    }
    if (mp.Rank() == 0 and write_mapbc_file) {
        std::vector<std::string> ugrid_extensions{"b8", "lb8", "ugrid"};
        std::string base_name = Parfait::StringTools::stripExtension(name, ugrid_extensions);
        Parfait::writeMapbcFile(base_name + ".mapbc", bc_map);
    }
}
