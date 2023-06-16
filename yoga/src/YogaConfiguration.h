#pragma once
#include <set>
#include <string>
#include <vector>
#include <MessagePasser/MessagePasser.h>

namespace YOGA {
class YogaConfiguration {
  public:
    YogaConfiguration() = delete;
    YogaConfiguration(MessagePasser mp);
    YogaConfiguration(const std::string& s);
    std::set<std::string> getOptionList() const;
    const std::set<int>& getRanksToTrace() const;
    std::string getTracerBaseName() const;
    int selectedLoadBalancer() const;
    int selectedTargetVoxelSize() const;
    int maxHoleMapCells() const;
    bool shouldAddExtraReceptors() const;
    bool shouldDumpStats() const;
    bool shouldUseZMQPath() const;
    int numberOfExtraLayersForInterpBcs() const;
    int rcbAgglomerationSize() const;
    bool shouldDumpPartFile() const;
    bool shouldDumpPartitionExtents() const;
    std::vector<int> getComponentGridImportance() const;

  private:
    int load_balancer_algorithm;
    int target_voxel_size;
    int max_hole_map_cells;
    int rcb_agglom_size;
    bool use_max_donors;
    bool should_dump_stats;
    bool should_use_zmq_path;
    bool should_dump_part_file;
    bool should_dump_partition_extents;
    int extra_receptors_for_interp_bcs;
    std::string trace_basename;
    std::set<int> ranks_to_trace;
    std::vector<int> grid_importance;
    const std::set<std::string> permissible_options;
    std::string loadFileToStringInParallel(MessagePasser mp, const std::string filename);
    bool isConfigFilePresent(std::string fname);
    bool isKeyWord(const std::string& word) const;
    void processKeyWord(int& index,const std::vector<std::string>& words);
    void processDumpCommand(const std::string& s);
    void configureWithString(const std::string& s);
    void setDefaults();
};
}
