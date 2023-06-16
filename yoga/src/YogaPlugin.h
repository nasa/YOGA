#pragma once

#include <t-infinity/DomainAssemblerInterface.h>
#include <t-infinity/FieldInterface.h>
#include <t-infinity/MeshInterface.h>
#include <functional>
#include "YogaMesh.h"
#include "BoundaryConditions.h"
#include "DruyorTypeAssignment.h"
#include "GhostSyncPatternBuilder.h"
#include "MeshInterfaceAdapter.h"
#include "OversetData.h"

class YogaPlugin : public inf::DomainAssemblerInterface {
  public:
    YogaPlugin(MessagePasser mp, const inf::MeshInterface& m, int component_id, std::string bc_string);

    virtual std::vector<int> performAssembly() override;

    void verifyDonors();

    std::vector<int> determineGridIdsForNodes() override;

    virtual std::map<int, std::vector<double>> updateReceptorSolutions(
        std::function<void(int, double, double, double, double*)> getter) const override;

    virtual std::vector<std::string> listFields() const override;

    virtual std::shared_ptr<inf::FieldInterface> field(std::string field_name) const override;

    MessagePasser mp;
    YOGA::YogaMesh mesh;
    std::vector<YOGA::NodeStatus> node_statuses;
    std::map<int, YOGA::OversetData::DonorCell> receptors;
  private:
    std::vector<int> framework_cell_ids;

    std::vector<YOGA::BoundaryConditions> createBoundaryConditionVector(const inf::MeshInterface& mesh,
                                                                        std::string input,
                                                                        int domain_id);

    bool contains(std::vector<int>& wall_tags, int tag) const {
        return find(wall_tags.begin(), wall_tags.end(), tag) != wall_tags.end();
    }

    int determineCellStatusBasedOnNodes(const std::vector<int>& cell,const std::vector<YOGA::NodeStatus>& statuses) const;

    int calcColorOffset(const std::vector<int>& colors) const;
};

class YogaPluginFactory : public inf::DomainAssemblerFactory {
  public:
    virtual std::shared_ptr<inf::DomainAssemblerInterface> createDomainAssembler(
        MPI_Comm comm, const inf::MeshInterface& mesh, int component_id, std::string json) const override;
    virtual ~YogaPluginFactory() = default;
};

extern "C" {
std::shared_ptr<inf::DomainAssemblerFactory> createDomainAssemblerFactory();
}
