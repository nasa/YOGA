#pragma once
#include <parfait/MapbcReader.h>
#include <vector>
#include <map>
#include <string>
#include <stdio.h>

namespace inf {
class MapBcCombiner {
  public:
    static void combine(const std::string& new_mapbc_filename,
                        const std::vector<std::string>& filenames) {
        std::vector<Parfait::BoundaryConditionMap> bc_maps;
        for (auto& file : filenames) {
            Parfait::MapbcReader reader(file);
            bc_maps.emplace_back(reader.getMap());
        }
        auto combined = combine(bc_maps);
        write(new_mapbc_filename, combined);
    }
    static Parfait::BoundaryConditionMap combine(
        const std::vector<Parfait::BoundaryConditionMap>& bc_maps) {
        Parfait::BoundaryConditionMap combined;
        int current_tag = 1;
        for (auto& bc_map : bc_maps) {
            for (auto& pair : bc_map) {
                combined[current_tag++] = pair.second;
            }
        }
        return combined;
    }

    static void write(const std::string& filename, const Parfait::BoundaryConditionMap& bc_map) {
        FILE* f = fopen(filename.c_str(), "w");
        fprintf(f, "%i\n", int(bc_map.size()));
        for (auto& pair : bc_map) {
            int tag = pair.first;
            int bc = pair.second.first;
            auto info_string = pair.second.second;
            fprintf(f, "%i %i %s\n", tag, bc, info_string.c_str());
        }
        fclose(f);
    }
};
}