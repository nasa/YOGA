#pragma once
#include <algorithm>
#include <t-infinity/Snap.h>

namespace inf {
std::shared_ptr<FieldInterface> loadMetricFromSnapFile(MessagePasser mp,
                                                       const MeshInterface& mesh,
                                                       const std::string snapfile) {
    Snap snap(mp.getCommunicator());
    snap.addMeshTopologies(mesh);
    snap.load(snapfile);
    auto available_fields = snap.availableFields();
    if (std::find(available_fields.begin(), available_fields.end(), "metric") ==
        available_fields.end()) {
        printf("Snap file does not contain a metric.\n");
        printf("Available fields:\n");
        for (auto s : available_fields) printf("   - %s\n", s.c_str());
        return nullptr;
    }
    return snap.retrieve("metric");
}

}