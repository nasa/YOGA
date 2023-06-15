#pragma once
#include <parfait/ToString.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/CellSelector.h>
#include <t-infinity/CellIdFilter.h>
#include <t-infinity/Shortcuts.h>

namespace inf {
struct MeshAndFields {
    std::shared_ptr<inf::MeshInterface> mesh;
    std::vector<std::shared_ptr<inf::FieldInterface>> fields;
};

class Transform {
  public:
    virtual MeshAndFields apply(MessagePasser mp, MeshAndFields mesh_and_fields) = 0;
    virtual ~Transform() = default;
};

class Pipeline : public Transform {
  public:
    MeshAndFields apply(MessagePasser mp, MeshAndFields mesh_and_fields) override {
        for (auto t : pipeline) {
            mesh_and_fields = t->apply(mp, mesh_and_fields);
        }
        return mesh_and_fields;
    }

    void append(std::shared_ptr<Transform> t) { pipeline.push_back(t); }

  private:
    std::vector<std::shared_ptr<Transform>> pipeline;
};

class MeshStatisticsPrinter : public Transform {
    MeshAndFields apply(MessagePasser mp, MeshAndFields mesh_and_fields) override {
        auto global_node_count = inf::globalNodeCount(mp, *mesh_and_fields.mesh);
        auto global_cell_count = inf::globalCellCount(mp, *mesh_and_fields.mesh);
        mp_rootprint("Mesh sizes: \n");
        mp_rootprint(" Node Count: %s\n",
                     Parfait::bigNumberToStringWithCommas(global_node_count).c_str());
        mp_rootprint(" Cell Count: %s\n",
                     Parfait::bigNumberToStringWithCommas(global_cell_count).c_str());
        return mesh_and_fields;
    }
};

class CellSelectorTransform : public Transform {
  public:
    CellSelectorTransform(std::shared_ptr<inf::CellSelector> cell_selector)
        : selector(cell_selector) {}
    MeshAndFields apply(MessagePasser mp, MeshAndFields mesh_and_fields) override {
        inf::CellIdFilter filter(mp.getCommunicator(), mesh_and_fields.mesh, selector);
        MeshAndFields out;
        out.mesh = filter.getMesh();
        for (auto f : mesh_and_fields.fields) {
            out.fields.push_back(filter.apply(f));
        }
        return out;
    }

  private:
    std::shared_ptr<inf::CellSelector> selector;
};
}
