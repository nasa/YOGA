#pragma once
#include <parfait/Inspector.h>

namespace YOGA{

    inline void printInspectorResults(Parfait::Inspector& inspector) {
        auto names = inspector.names();
        printf("max mean imbalance name\n");
        for (auto& name : names) {
            double mean = inspector.mean(name);
            double max = inspector.max(name);
            double imbalance = inspector.imbalance(name);
            printf("%.2lf %.2lf %.2lf %s\n", max, mean, imbalance, name.c_str());
        }
    }
}
