#pragma once
#include "MapbcParser.h"
#include "Throw.h"

namespace Parfait {
inline void writeMapbcFile(const std::string filename, const Parfait::BoundaryConditionMap& bc_map) {
    FILE* f = fopen(filename.c_str(), "w");
    PARFAIT_ASSERT(f!=nullptr, "Couldn't open file: " + filename + " for writing");
    fprintf(f, "%i\n", int(bc_map.size()));
    for (auto& pair : bc_map) {
        int tag = pair.first;
        int bc = pair.second.first;
        auto info_string = pair.second.second;
        fprintf(f, "%i %i %s\n", tag, bc, info_string.c_str());
    }
    fclose(f);
}
}
