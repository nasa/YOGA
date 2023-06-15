#pragma once
#include <vector>
#include <algorithm>
#include <MessagePasser/MessagePasser.h>
#include <Tracer.h>
namespace Parfait {

namespace Deprecated {

    class GlobalCellIdBuilder {
      public:
        static std::vector<long> buildGlobalCellIds(const std::vector<std::vector<long>>& cells,
                                                    const std::vector<long>& global_node_ids,
                                                    const std::vector<int>& cell_owners,
                                                    MessagePasser& mp) {
            long offset = GlobalCellIdBuilder::calcGlobalCellIdOffset(mp, cell_owners);

            auto global_cell_ids = GlobalCellIdBuilder::calcOwnedGlobalCellIds(cell_owners, offset, mp.Rank());

            GlobalCellIdBuilder::syncCell(global_cell_ids, cells, cell_owners, mp);

            return global_cell_ids;
        }

        static int inferCellOwnerFromNodeOwners(const std::vector<long>& gids, const std::vector<int>& owners) {
            int index_of_lowest = 0;
            for (unsigned int i = 1; i < gids.size(); i++)
                if (gids[i] < gids[index_of_lowest]) index_of_lowest = i;
            return owners[index_of_lowest];
        }

        static int countOwnedCells(int rank, const std::vector<int>& owners) {
            int nlocal_cells = 0;
            for (auto owner : owners)
                if (rank == owner) nlocal_cells++;
            return nlocal_cells;
        }

        static long calcOffsetFromCounts(int rank, const std::vector<long>& counts) {
            long offset = 0;
            for (long i = 0; i < rank; i++) offset += counts[i];
            return offset;
        }

        static long calcGlobalCellIdOffset(MessagePasser mp, const std::vector<int>& owners) {
            long nlocal_cells = countOwnedCells(mp.Rank(), owners);
            std::vector<long> counts(mp.NumberOfProcesses(), 0);
            mp.Gather(nlocal_cells, counts);
            return calcOffsetFromCounts(mp.Rank(), counts);
        }

        static std::vector<long> calcOwnedGlobalCellIds(const std::vector<int>& cell_owners, long offset, int rank) {
            std::vector<long> ids(cell_owners.size(), -1);
            long current_cell_id = offset;
            for (unsigned int i = 0; i < cell_owners.size(); i++) {
                if (rank == cell_owners[i]) {
                    ids[i] = current_cell_id;
                    current_cell_id++;
                }
            }
            return ids;
        }

        static std::vector<long> packageCellsForRank(int target_rank,
                                                     const std::vector<std::vector<long>>& cells,
                                                     const std::vector<int>& owners) {
            std::vector<long> message;
            for (unsigned int i = 0; i < owners.size(); i++) {
                if (owners[i] == target_rank) {
                    message.push_back(cells[i].size());
                    for (long id : cells[i]) message.push_back(id);
                }
            }
            return message;
        }

        static std::vector<std::vector<long>> extractCellsFromMessage(const std::vector<long>& message) {
            // Tracer::begin("extractCellsFromMessage");
            std::vector<std::vector<long>> cells;
            int index = 0;
            size_t remaining_message_length = message.size() - index;
            while (remaining_message_length > 0) {
                std::vector<long> cell;
                long cell_size = message[index++];
                for (long i = 0; i < cell_size; i++) cell.push_back(message[index++]);
                cells.emplace_back(cell);
                remaining_message_length = message.size() - index;
            }
            // Tracer::end("extractCellsFromMessage");
            return cells;
        }

        static std::map<std::vector<long>, long> buildCellMap(const std::vector<std::vector<long>>& cells,
                                                              const std::vector<long>& cell_ids) {
            std::map<std::vector<long>, long> cell_map;
            for (unsigned int i = 0; i < cells.size(); i++) {
                auto cell = cells[i];
                std::sort(cell.begin(), cell.end());
                cell_map[cell] = cell_ids[i];
            }
            return cell_map;
        };

        static std::vector<long> getGlobalIdsForCells(const std::vector<std::vector<long>>& requested_cells,
                                                      const std::vector<std::vector<long>>& cells,
                                                      const std::vector<long>& cell_ids) {
            auto cell_map = buildCellMap(cells, cell_ids);

            std::vector<long> ids;
            for (auto cell : requested_cells) {
                std::sort(cell.begin(), cell.end());
                if (cell_map.count(cell) == 0) {
                    printf("Searching for cell: ");
                    for (long id : cell) printf("%lu ", id);
                    printf("\nCell map:\n");
                    for (auto& pair : cell_map) {
                        printf("key: ");
                        for (long id : pair.first) printf("%lu ", id);
                        printf("\nvalue: %lu\n", pair.second);
                    }
                    throw std::logic_error(std::string(__FILE__) + " asked for non-owned cell");
                }
                ids.push_back(cell_map[cell]);
            }
            return ids;
        }

        static void syncCell(std::vector<long>& cell_ids,
                             const std::vector<std::vector<long>>& cells,
                             const std::vector<int>& owners,
                             MessagePasser& mp) {
            std::vector<std::vector<long>> my_messages;
            for (int i = 0; i < mp.NumberOfProcesses(); i++) {
                auto message = packageCellsForRank(i, cells, owners);
                mp.Gather(message, my_messages, i);
            }

            std::vector<std::vector<long>> responses;
            for (auto& message : my_messages) {
                auto requested_cells = extractCellsFromMessage(message);
                responses.emplace_back(getGlobalIdsForCells(requested_cells, cells, cell_ids));
            }

            std::vector<std::vector<long>> ghost_cell_ids;
            for (int i = 0; i < mp.NumberOfProcesses(); i++) {
                mp.Gather(responses[i], ghost_cell_ids, i);
            }

            for (unsigned int i = 0; i < cells.size(); i++) {
                if (cell_ids[i] == -1) {
                    auto& ghost_ids_from_rank = ghost_cell_ids[owners[i]];
                    cell_ids[i] = ghost_ids_from_rank.front();
                    ghost_ids_from_rank.erase(ghost_ids_from_rank.begin(), ghost_ids_from_rank.begin() + 1);
                }
            }
        }
    };
}
}
