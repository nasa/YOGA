#pragma once
#include <parfait/Point.h>
#include <t-infinity/TinfMesh.h>
#include <map>
#include <vector>
#include "NC_PartitionerBase.h"
#include "Redistributor.h"

namespace NodeCentered {
class PProcessor {
  public:
    PProcessor(MessagePasser mp, std::shared_ptr<Reader> reader, NC::Partitioner& partitioner)
        : mp(mp), reader(reader), partitioner(partitioner), distributor(mp, reader) {}

    NaiveMesh loadNaiveMesh() {
        mp_rootprint("PP: Loading naive mesh\n");
        mp_rootprint("PP: distributing nodes\n");
        auto coords = distributor.distributeNodes();
        mp_rootprint("PP: distributing cell types\n");

        auto cell_types = distributor.cellTypes();
        std::map<inf::MeshInterface::CellType, std::vector<long>> cell_collection;
        std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
        std::map<inf::MeshInterface::CellType, std::vector<long>> global_cell_ids;
        for (auto& cell_type : cell_types) {
            std::string s =
                "PP: distributing cells of type " + std::string(inf::MeshInterface::cellTypeString(cell_type));
            mp_rootprint("%s\n", s.c_str());
            std::vector<long> cells;
            std::vector<int> cell_tags_buffer;
            std::vector<long> cell_global_ids;
            std::tie(cells, cell_tags_buffer, cell_global_ids) =
                distributor.distributeCellsTagsAndGlobalIds2(cell_type);
            if (cell_tags_buffer.size() != cell_global_ids.size())
                throw std::logic_error("Cell tags and global ids are mismatched");
            cell_collection[cell_type] = cells;
            cell_tags[cell_type] = cell_tags_buffer;
            global_cell_ids[cell_type] = cell_global_ids;
        }
        auto node_range = distributor.nodeRange();
        std::vector<long> global_node_id;
        for (long id = node_range.start; id < node_range.end; id++) {
            global_node_id.push_back(id);
        }
        mp_rootprint("PP: done\n");
        return NaiveMesh(coords, cell_collection, cell_tags, global_cell_ids, global_node_id, node_range);
    }

    std::shared_ptr<inf::TinfMesh> createMesh() {
        auto nm = loadNaiveMesh();

        auto part = partitioner.generatePartVector(mp, nm);

        mp_rootprint("about to start redistributing\n");
        Redistributor redistributor(mp, std::move(nm), std::move(part));
        mp_rootprint("about to start creating mesh\n");
        auto result = redistributor.createMesh();
        return result;
    }

  private:
    MessagePasser mp;
    std::shared_ptr<Reader> reader;
    const NC::Partitioner& partitioner;
    Distributor distributor;
};
}
