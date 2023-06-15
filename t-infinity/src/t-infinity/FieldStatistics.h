#pragma once
#include <MessagePasser/MessagePasser.h>
#include "MeshInterface.h"

namespace inf {
namespace FieldStatistics {
    struct Statistics {
        double avg;
        double min;
        double max;
    };

    inline void print(MessagePasser mp, const Statistics& s, std::string name = "") {
        mp_rootprint(" %s: min %1.2e, max %e, avg %1.3e\n", name.c_str(), s.min, s.max, s.avg);
    }

    inline Statistics calcCells(MessagePasser mp,
                                const inf::MeshInterface& mesh,
                                const std::vector<double>& field) {
        Statistics s;
        s.avg = 0.0;
        s.min = 2e200;
        s.max = -2e200;

        long count = 0;
        for (int c = 0; c < mesh.cellCount(); c++) {
            double f = field[c];
            s.avg += f;
            s.min = std::min(s.min, f);
            s.max = std::max(s.max, f);
            count++;
        }

        s.min = mp.ParallelMin(s.min);
        s.max = mp.ParallelMax(s.max);
        s.avg = mp.ParallelSum(s.avg);
        count = mp.ParallelSum(count);

        s.avg /= double(count);
        return s;
    }
    inline Statistics calcVolumeCells(MessagePasser mp,
                                      const inf::MeshInterface& mesh,
                                      const std::vector<double>& field) {
        Statistics s;
        s.avg = 0.0;
        s.min = 2e200;
        s.max = -2e200;

        long count = 0;
        for (int c = 0; c < mesh.cellCount(); c++) {
            auto type = mesh.cellType(c);
            if (mesh.is3DCellType(type)) {
                double f = field[c];
                s.avg += f;
                s.min = std::min(s.min, f);
                s.max = std::max(s.max, f);
                count++;
            }
        }

        s.min = mp.ParallelMin(s.min);
        s.max = mp.ParallelMax(s.max);
        s.avg = mp.ParallelSum(s.avg);
        count = mp.ParallelSum(count);

        s.avg /= double(count);
        return s;
    }
    inline Statistics calcNodes(MessagePasser mp,
                                const inf::MeshInterface& mesh,
                                const std::vector<double>& field) {
        Statistics s;
        s.avg = 0.0;
        s.min = 2e200;
        s.max = -2e200;

        long count = 0;
        for (int n = 0; n < mesh.nodeCount(); n++) {
            double f = field[n];
            s.avg += f;
            s.min = std::min(s.min, f);
            s.max = std::max(s.max, f);
            count++;
        }

        s.min = mp.ParallelMin(s.min);
        s.max = mp.ParallelMax(s.max);
        s.avg = mp.ParallelSum(s.avg);
        count = mp.ParallelSum(count);

        s.avg /= double(count);
        return s;
    }
}
}
