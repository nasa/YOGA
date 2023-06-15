#include "YogaConfiguration.h"
#include <parfait/FileTools.h>
#include <parfait/StringTools.h>
#include <unistd.h>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <parfait/Checkpoint.h>
#include <parfait/ToString.h>

namespace YOGA {

std::string YogaConfiguration::loadFileToStringInParallel(MessagePasser mp, const std::string filename){
    std::string s;
    if(mp.Rank() == 0) {
        if (isConfigFilePresent(filename)) {
            s = Parfait::FileTools::loadFileToString(filename);
        }
    }
    mp.Broadcast(s,0);
    return s;
}

YogaConfiguration::YogaConfiguration(MessagePasser mp) : permissible_options(getOptionList()) {
    setDefaults();
    std::string config_filename = "yoga.config";
    auto s = loadFileToStringInParallel(mp,config_filename);
    configureWithString(s);
}

const std::set<int>& YogaConfiguration::getRanksToTrace() const { return ranks_to_trace; }
std::string YogaConfiguration::getTracerBaseName() const { return trace_basename;}
int YogaConfiguration::selectedLoadBalancer() const { return load_balancer_algorithm; }
int YogaConfiguration::selectedTargetVoxelSize() const { return target_voxel_size; }
bool YogaConfiguration::shouldAddExtraReceptors() const { return use_max_donors; }

bool YogaConfiguration::isConfigFilePresent(std::string fname) {
    std::ifstream f(fname.c_str());
    return f.good();
}
int YogaConfiguration::numberOfExtraLayersForInterpBcs() const { return extra_receptors_for_interp_bcs; }

static std::vector<std::string> tokenize(const std::string& config) {
    std::string s = config;
    s = Parfait::StringTools::findAndReplace(s, "\t", " ");
    s = Parfait::StringTools::findAndReplace(s, "\n", " ");
    return Parfait::StringTools::split(s, " ");
}

YogaConfiguration::YogaConfiguration(const std::string& s) : permissible_options(getOptionList()) {
    setDefaults();
    configureWithString(s);
}

void YogaConfiguration::configureWithString(const std::string& s) {
    auto key_words = permissible_options;
    auto conf = Parfait::StringTools::removeCommentLines(s, "#");
    auto words = tokenize(conf);
    auto stuff = Parfait::to_string(words);
    for (int i = 0; i < long(words.size()); i++) {
        if (key_words.count(words[i]) == 1) {
            processKeyWord(i, words);
        } else {
            throw std::logic_error("Unexpected keyword:(" + words[i] + ")");
        }
    }
}

bool YogaConfiguration::isKeyWord(const std::string& word) const { return permissible_options.count(word) == 1; }

void YogaConfiguration::processKeyWord(int& index, const std::vector<std::string>& words) {
    auto keyword = words[index];
    if ("trace" == keyword) {
        while (++index < long(words.size())) {
            auto word = words[index];
            if (isKeyWord(word)) break;
            if (Parfait::StringTools::isInteger(word)) {
                ranks_to_trace.insert(std::atoi(word.c_str()));
            }
        }
        --index;
    } else if ("extra-layers-for-interpolation-bcs" == keyword) {
        auto word = words[++index];
        if (Parfait::StringTools::isInteger(word)) {
            extra_receptors_for_interp_bcs = std::atoi(word.c_str());
        }
    } else if ("target-voxel-size" == keyword) {
        auto word = words[++index];
        if (Parfait::StringTools::isInteger(word)) {
            target_voxel_size = std::atoi(word.c_str());
        }
    } else if("max-hole-map-cells" == keyword) {
        auto word = words[++index];
        if (Parfait::StringTools::isInteger(word)) {
            max_hole_map_cells = std::atoi(word.c_str());
        }
    } else if ("max-receptors" == keyword) {
        use_max_donors = true;
    } else if ("load-balancer" == keyword) {
        auto word = words[++index];
        if (Parfait::StringTools::isInteger(word)) {
            load_balancer_algorithm = std::atoi(word.c_str());
        }
    } else if ("dump-stats" == keyword) {
        should_dump_stats = true;
    } else if ("zmq-path" == keyword) {
        should_use_zmq_path = true;
    }
    else if("trace-basename" == keyword){
        trace_basename = words[++index];
    }
    else if("rcb" == keyword){
        auto word = words[++index];
        if (Parfait::StringTools::isInteger(word)) {
            rcb_agglom_size = std::atoi(word.c_str());
        }
    }
    else if("dump" == keyword){
        processDumpCommand(words[++index]);
    }
    else if("component-grid-importance" == keyword){
        while (++index < long(words.size())) {
            auto word = words[index];
            if (isKeyWord(word)) break;
            if (Parfait::StringTools::isInteger(word)) {
                grid_importance.push_back(std::atoi(word.c_str()));
            }
        }
        --index;
    }
    else{
        throw std::logic_error("Unrecognized key: "+keyword);
    }
}

void YogaConfiguration::processDumpCommand(const std::string& s) {
    if("fun3d-part-file" == s){
        should_dump_part_file = true;
    }
    else if("partition-extents" == s){
        should_dump_partition_extents = true;
    }
    else{
        throw std::logic_error("Unrecognized command: dump "+s);
    }
}

std::set<std::string> YogaConfiguration::getOptionList() const {
    return {"trace",
            "donor-algorithm",
            "max-receptors",
            "max-hole-map-cells",
            "load-balancer",
            "target-voxel-size",
            "dump-stats",
            "zmq-path",
            "extra-layers-for-interpolation-bcs",
            "trace-basename",
            "rcb",
            "dump",
            "component-grid-importance"
    };
}
void YogaConfiguration::setDefaults() {
    use_max_donors = false;
    load_balancer_algorithm = 0;
    max_hole_map_cells = 8000;
    target_voxel_size = 25000;
    extra_receptors_for_interp_bcs = 1;
    should_dump_stats = false;
    should_use_zmq_path = false;
    should_dump_part_file = false;
    trace_basename = "yoga";
    rcb_agglom_size = 256;
}
bool YogaConfiguration::shouldDumpStats() const { return should_dump_stats; }
bool YogaConfiguration::shouldUseZMQPath() const { return should_use_zmq_path; }
int YogaConfiguration::rcbAgglomerationSize() const {
    return rcb_agglom_size;
}
bool YogaConfiguration::shouldDumpPartFile() const {
    return should_dump_part_file;
}

bool YogaConfiguration::shouldDumpPartitionExtents() const {
    return should_dump_partition_extents;
}
std::vector<int> YogaConfiguration::getComponentGridImportance() const {
    return grid_importance;
}
int YogaConfiguration::maxHoleMapCells() const { return max_hole_map_cells; }
}