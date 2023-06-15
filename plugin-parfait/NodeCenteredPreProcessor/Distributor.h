#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/Point.h>
#include <parfait/ToString.h>
#include <t-infinity/MeshInterface.h>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include "../shared/Reader.h"

class Distributor {
  public:
    Distributor(MessagePasser mp, std::shared_ptr<Reader> reader) : mp(mp), reader(reader), first_cell_id_in_chunk(0) {
        if (mp.Rank() == 0) {
            global_node_count = reader->nodeCount();
            printf("Loading mesh with %s nodes.\n", Parfait::bigNumberToStringWithCommas(global_node_count).c_str());
        }
        mp.Broadcast(global_node_count, 0);
    }

    long globalNodeCount() const { return global_node_count; }

    void determineCellOwners(const long* cell, int length, std::set<int>& owners) const {
        for (int i = 0; i < length; i++) {
            long gid = cell[i];
            int owner = Parfait::LinearPartitioner::getWorkerOfWorkItem(gid, global_node_count, mp.NumberOfProcesses());
            owners.insert(owner);
        }
    }

    std::vector<Parfait::LinearPartitioner::Range<long>> buildNodeRangePerRank(int num_ranks, long node_count) const {
        std::vector<Parfait::LinearPartitioner::Range<long>> node_range_per_rank(num_ranks);
        for (int r = 0; r < num_ranks; r++) {
            node_range_per_rank[r] = Parfait::LinearPartitioner::getRangeForWorker(r, node_count, num_ranks);
        }
        return node_range_per_rank;
    }

    std::vector<Parfait::Point<double>> distributeNodes() const {
        auto node_range_per_rank = buildNodeRangePerRank(mp.NumberOfProcesses(), global_node_count);

        auto my_range = node_range_per_rank[mp.Rank()];
        std::vector<Parfait::Point<double>> my_nodes((my_range.end - my_range.start));
        int max_send_buffers = 8;
        using Buffer = std::vector<Parfait::Point<double>>;
        std::queue<std::shared_ptr<Buffer>> send_buffers;
        std::queue<MessagePasser::MessageStatus> statuses;
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            if (mp.Rank() == 0) {
                auto range = node_range_per_rank[r];
                if (statuses.size() >= size_t(max_send_buffers)) {
                    auto& status = statuses.front();
                    status.wait();
                    statuses.pop();
                    send_buffers.pop();
                }
                auto send_buffer_ptr = std::make_shared<Buffer>(reader->readCoords(range.start, range.end));
                send_buffers.push(send_buffer_ptr);
                statuses.push(mp.NonBlockingSend(*send_buffer_ptr, int(send_buffer_ptr->size()), r));
            }
            if (r == mp.Rank()) {
                int nnodes = my_range.end - my_range.start;
                my_nodes.resize(nnodes);
                mp.Recv(my_nodes, nnodes, 0);
            }
        }

        if (mp.Rank() == 0) {
            while (statuses.size() > 0) {
                auto& s = statuses.front();
                s.wait();
                statuses.pop();
                send_buffers.pop();
            }
        }

        return my_nodes;
    }

    std::tuple<std::vector<long>, std::vector<int>, std::vector<long>> distributeCellsTagsAndGlobalIds2(
        inf::MeshInterface::CellType type) {
        int cell_length = inf::MeshInterface::cellTypeLength(type);
        std::vector<long> cells;
        std::vector<int> tags;
        std::vector<long> cell_ids;
        long cell_count = 0;
        if (mp.Rank() == 0) {
            cell_count = reader->cellCount(type);
            printf("There are %s %s\n",
                   Parfait::bigNumberToStringWithCommas(cell_count).c_str(),
                   inf::MeshInterface::cellTypeString(type).c_str());
            fflush(stdout);
        }
        mp.Broadcast(cell_count, 0);

        int nchunks = mp.NumberOfProcesses();
        auto my_range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(), cell_count, nchunks);
        int my_cell_count = my_range.end - my_range.start;
        cells.resize(cell_length * my_cell_count);
        tags.resize(my_cell_count);
        cell_ids.resize(my_cell_count);
        auto s1 = mp.NonBlockingRecv(cells, cell_length * my_cell_count, 0);
        auto s2 = mp.NonBlockingRecv(tags, my_cell_count, 0);
        if (mp.Rank() == 0) {
            std::queue<std::shared_ptr<std::vector<long>>> cell_buffers;
            std::queue<std::shared_ptr<std::vector<int>>> tag_buffers;
            std::queue<MessagePasser::MessageStatus> promises;
            for (int chunk = 0; chunk < nchunks; chunk++) {
                auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk, cell_count, nchunks);
                int ncells = range.end - range.start;
                auto cell_buffer = reader->readCells(type, range.start, range.end);
                auto tag_buffer = reader->readCellTags(type, range.start, range.end);
                auto cells_ptr = std::make_shared<std::vector<long>>(std::move(cell_buffer));
                auto tags_ptr = std::make_shared<std::vector<int>>(std::move(tag_buffer));
                int target_rank = chunk;
                promises.push(mp.NonBlockingSend(*cells_ptr, cell_length * ncells, target_rank));
                promises.push(mp.NonBlockingSend(*tags_ptr, ncells, target_rank));
                cell_buffers.push(cells_ptr);
                tag_buffers.push(tags_ptr);
                while (promises.size() > 10) {
                    for (int i = 0; i < 2; i++) {
                        auto p = promises.front();
                        p.wait();
                        promises.pop();
                    }
                    cell_buffers.pop();
                    tag_buffers.pop();
                }
            }
            while (not promises.empty()) {
                auto p = promises.front();
                p.wait();
                promises.pop();
            }
        }

        for (int chunk = 0; chunk < mp.Rank(); chunk++) {
            auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk, cell_count, nchunks);
            int ncells = range.end - range.start;
            first_cell_id_in_chunk += ncells;
        }
        long my_first_cell_id = first_cell_id_in_chunk;
        for (int chunk = mp.Rank(); chunk < nchunks; chunk++) {
            auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk, cell_count, nchunks);
            int ncells = range.end - range.start;
            first_cell_id_in_chunk += ncells;
        }
        for (int i = 0; i < my_cell_count; i++) {
            cell_ids[i] = my_first_cell_id++;
        }

        s1.wait();
        s2.wait();

        std::vector<std::vector<long>> cells_for_ranks(mp.NumberOfProcesses());
        std::vector<std::vector<int>> tags_for_ranks(mp.NumberOfProcesses());
        std::vector<std::vector<long>> cell_ids_for_ranks(mp.NumberOfProcesses());
        std::set<int> owners_of_this_cell;
        for (int i = 0; i < my_cell_count; i++) {
            auto cell_ptr = &cells[cell_length * i];
            owners_of_this_cell.clear();
            determineCellOwners(cell_ptr, cell_length, owners_of_this_cell);
            for (int owning_rank : owners_of_this_cell) {
                auto& vec = cells_for_ranks[owning_rank];
                vec.insert(vec.end(), cell_ptr, cell_ptr + cell_length);
                tags_for_ranks[owning_rank].push_back(tags[i]);
                cell_ids_for_ranks[owning_rank].push_back(cell_ids[i]);
            }
        }
        cells.clear();
        tags.clear();
        cell_ids.clear();
        auto cells_from_ranks = mp.Exchange(cells_for_ranks);
        auto tags_from_ranks = mp.Exchange(tags_for_ranks);
        auto ids_from_ranks = mp.Exchange(cell_ids_for_ranks);
        cells_for_ranks.clear();
        cells_for_ranks.shrink_to_fit();
        tags_for_ranks.clear();
        tags_for_ranks.shrink_to_fit();
        cell_ids_for_ranks.clear();
        cell_ids_for_ranks.shrink_to_fit();

        for (auto& v : cells_from_ranks) {
            cells.insert(cells.end(), v.begin(), v.end());
        }
        for (auto& v : tags_from_ranks) {
            tags.insert(tags.end(), v.begin(), v.end());
        }
        for (auto& v : ids_from_ranks) {
            cell_ids.insert(cell_ids.end(), v.begin(), v.end());
        }
        return std::tie(cells, tags, cell_ids);
    }

    Parfait::LinearPartitioner::Range<long> nodeRange() const {
        auto range =
            Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(), global_node_count, mp.NumberOfProcesses());
        return range;
    }

    std::vector<inf::MeshInterface::CellType> cellTypes() const { return reader->cellTypes(); }

  private:
    MessagePasser mp;
    std::shared_ptr<Reader> reader;
    long global_node_count;
    long first_cell_id_in_chunk;
};
