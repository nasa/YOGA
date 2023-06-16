#pragma once
#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Throw.h>
#include <vector>
#include <set>
#include <iostream>

namespace inf {
class PartitionConnectivityMapper {
  public:
    static std::vector<std::vector<int>> buildPartToPart(const MeshInterface& mesh,
                                                         MessagePasser mp) {
        std::set<int> nbrs;
        for (int i = 0; i < mesh.nodeCount(); i++) nbrs.insert(mesh.nodeOwner(i));
        std::vector<int> my_nbrs(nbrs.begin(), nbrs.end());

        std::vector<std::vector<int>> all_nbrs;
        mp.Gather(my_nbrs, all_nbrs);
        return all_nbrs;
    }

    static std::vector<std::vector<int>> buildParallelPartToPart(const MeshInterface& mesh,
                                                                 MessagePasser mp) {
        std::set<int> nbrs;
        for (int i = 0; i < mesh.nodeCount(); i++) nbrs.insert(mesh.nodeOwner(i));
        std::vector<int> my_nbrs(nbrs.begin(), nbrs.end());

        std::vector<std::vector<int>> all_nbrs;
        all_nbrs.emplace_back(my_nbrs);
        return all_nbrs;
    }

    static std::vector<std::vector<int>> buildCPUToParts(const std::vector<int>& part_to_cpu) {
        int max_cpu_id = 0;
        for (int id : part_to_cpu) max_cpu_id = std::max(id, max_cpu_id);
        std::vector<std::vector<int>> partitions_per_cpu(max_cpu_id + 1);

        for (size_t i = 0; i < part_to_cpu.size(); i++) {
            int owner = part_to_cpu[i];
            int part_id = i;
            partitions_per_cpu[owner].push_back(part_id);
        }
        return partitions_per_cpu;
    }

    static std::vector<int> buildOldPartToNew(const std::vector<int>& orig_part_to_cpu) {
        auto cpu_to_parts = buildCPUToParts(orig_part_to_cpu);
        std::vector<int> old_to_new(orig_part_to_cpu.size());
        for (size_t i = 0; i < orig_part_to_cpu.size(); i++) {
            int old_part_id = i;
            int cpu_id = orig_part_to_cpu[old_part_id];
            int new_part_id = determineNewPartitionId(cpu_id, cpu_to_parts[cpu_id], old_part_id);
            old_to_new[old_part_id] = new_part_id;
        }
        return old_to_new;
    }

    static std::vector<std::vector<int>> buildCPUToParts(std::vector<int>& part_to_cpu,
                                                         int num_partitions_per_group) {
        int max_cpu_id = 0;
        for (int id : part_to_cpu) max_cpu_id = std::max(id, max_cpu_id);
        // std::cout << "max_cpu_id = " << max_cpu_id << " part_to_cpu.size() = " <<
        // part_to_cpu.size() << std::endl;
        std::vector<std::vector<int>> partitions_per_cpu(max_cpu_id + 1);

        std::vector<int> unassigned;
        for (size_t i = 0; i < part_to_cpu.size(); i++) {
            int owner = part_to_cpu[i];
            int part_id = i;
            if (partitions_per_cpu[owner].size() < static_cast<size_t>(num_partitions_per_group)) {
                partitions_per_cpu[owner].push_back(part_id);
            } else {
                unassigned.push_back(part_id);
            }
        }
        // std::cout << " unassigned.size() = " << unassigned.size() << std::endl;
        for (size_t u = 0; u < unassigned.size(); u++) {
            // std::cout << " second pass to assign " << unassigned[u] << std::endl;
            for (size_t i = 0; i < part_to_cpu.size(); i++) {
                if (partitions_per_cpu[i].size() < static_cast<size_t>(num_partitions_per_group)) {
                    partitions_per_cpu[i].push_back(unassigned[u]);
                    part_to_cpu[unassigned[u]] = i;
                    // std::cout << unassigned[u] << " assigned to " << i << std::endl;
                    unassigned[u] = -1;
                    break;
                }
            }
        }
        // for (size_t u = 0; u < unassigned.size(); u++) {
        //     if (unassigned[u] != -1) {
        //         std::cout << "ERROR unassigned[" << u << "] = " << unassigned[u] << std::endl;
        //     }
        // }
        // for (size_t i = 0; i < partitions_per_cpu.size(); i++) {
        //     std::cout << " partitions_per_cpu[" << i << "].size() = " <<
        //     partitions_per_cpu[i].size() << std::endl;
        // }
        return partitions_per_cpu;
    }

    static std::vector<int> buildOldPartToNew(std::vector<int>& orig_part_to_cpu,
                                              int num_partitions_per_group) {
        auto cpu_to_parts = buildCPUToParts(orig_part_to_cpu, num_partitions_per_group);
        std::vector<int> old_to_new(orig_part_to_cpu.size());
        for (size_t i = 0; i < orig_part_to_cpu.size(); i++) {
            int old_part_id = i;
            int cpu_id = orig_part_to_cpu[old_part_id];
            int new_part_id = determineNewPartitionId(cpu_id, cpu_to_parts[cpu_id], old_part_id);
            old_to_new[old_part_id] = new_part_id;
        }
        return old_to_new;
    }

  private:
    static int determineNewPartitionId(int cpu_id,
                                       const std::vector<int>& parts_on_cpu,
                                       int old_part_id) {
        int new_part_id = old_part_id;
        int nranks_per_cpu = parts_on_cpu.size();
        bool found = false;
        for (int j = 0; j < nranks_per_cpu; j++) {
            if (old_part_id == parts_on_cpu[j]) {
                new_part_id = nranks_per_cpu * cpu_id + j;
                found = true;
            }
        }
        if (!found) {
            std::string error = "Cannot reassign partition id within group: ";
            PARFAIT_THROW(error + "old_part_id = " + std::to_string(old_part_id) +
                          " was not found in parts_on_cpu[" + std::to_string(cpu_id) + "]");
        }
        return new_part_id;
    }
};
}  // namespace inf