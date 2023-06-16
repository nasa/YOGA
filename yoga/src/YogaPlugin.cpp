#include "YogaPlugin.h"
#include <YogaConfiguration.h>
#include <Tracer.h>
#include <t-infinity/VectorFieldAdapter.h>
#include "AssemblyViaExchange.h"
#include "BoundaryConditionParser.h"
#include <parfait/CellContainmentChecker.h>
#include <parfait/Checkpoint.h>
#include "DistanceFieldAdapter.h"
#include "ParallelSurface.h"
#include "ReceptorUpdate.h"
#include "GraphColoring.h"
#include "ParallelColorCombinator.h"
#include "ColorSyncer.h"
#include "ComponentGridIdentifier.h"
#include "LinearTestFunction.h"

#ifdef YOGA_WITH_ZMQ
#include "AssemblyViaZMQPostMan.h"
#include "ZMQMessager.h"
#endif

using namespace inf;
using namespace YOGA;

YogaPlugin::YogaPlugin(MessagePasser mp, const MeshInterface& m, int component_id, std::string bc_string)
    : mp(mp), mesh() {
    YOGA::MeshInterfaceAdapter adapter(mp, m, component_id, createBoundaryConditionVector(m, bc_string, component_id));

    std::set<int> available_tags;
    for (int i = 0; i < m.cellCount(); i++) {
        if (m.is2DCell(i)) {
            int tag = m.cellTag(i);
            available_tags.insert(tag);
        }
    }
    mesh.setNodeCount(adapter.numberOfNodes());
    mesh.setXyzForNodes([&](int id, double* xyz) {
        auto p = adapter.getNode(id);
        xyz[0] = p[0];
        xyz[1] = p[1];
        xyz[2] = p[2];
    });
    mesh.setGlobalNodeIds([&](int id) { return adapter.getGlobalNodeId(id); });
    mesh.setOwningRankForNodes([&](int id) { return adapter.getNodeOwner(id); });
    mesh.setComponentIdsForNodes([&](int id) { return adapter.getAssociatedComponentId(id); });

    mesh.setCellCount(adapter.numberOfCells());
    auto get_cell_size = [&](int id) { return adapter.numberOfNodesInCell(id); };
    auto get_cell_nodes = [&](int id, int* cell) { adapter.getNodesInCell(id, cell); };
    mesh.setCells(get_cell_size, get_cell_nodes);
    framework_cell_ids.resize(adapter.numberOfCells());
    for (int i = 0; i < adapter.numberOfCells(); i++) {
        framework_cell_ids[i] = adapter.frameworkCellId(i);
    }

    mesh.setFaceCount(adapter.numberOfBoundaryFaces());
    auto get_face_size = [&](int id) { return adapter.getNodesInBoundaryFace(id).size(); };
    auto get_face_nodes = [&](int id, int* face_ptr) {
        auto face = adapter.getNodesInBoundaryFace(id);
        std::copy(face.begin(), face.end(), face_ptr);
    };
    mesh.setFaces(get_face_size, get_face_nodes);
    mesh.setBoundaryConditions([&](int id) { return adapter.getBoundaryCondition(id); }, get_face_size);
}

std::vector<int> YogaPlugin::performAssembly() {
    auto config = YOGA::YogaConfiguration(mp);
    mp.Barrier();
    std::shared_ptr<YOGA::OversetData> overset_data = nullptr;

    int load_balancer_algorithm = config.selectedLoadBalancer();
    int target_voxel_size = config.selectedTargetVoxelSize();
    int extra_layers = config.numberOfExtraLayersForInterpBcs();
    int rcb_agglom_ncells = config.rcbAgglomerationSize();
    bool should_add_max_receptors = config.shouldAddExtraReceptors();
    auto component_grid_importance = config.getComponentGridImportance();

    if (config.shouldUseZMQPath()) {
#ifdef YOGA_WITH_ZMQ
        overset_data = YOGA::assemblyViaZMQPostMan(mp,
                                                   mesh,
                                                   load_balancer_algorithm,
                                                   target_voxel_size,
                                                   extra_layers,
                                                   rcb_agglom_ncells,
                                                   should_add_max_receptors,
                                                   &Parfait::CellContainmentChecker::isInCell_c);
#else
        printf("Not configured with zmq");
        return {};
#endif
    } else {
        overset_data = YOGA::assemblyViaExchange(mp,
                                                 mesh,
                                                 load_balancer_algorithm,
                                                 target_voxel_size,
                                                 extra_layers,
                                                 rcb_agglom_ncells,
                                                 should_add_max_receptors,
                                                 component_grid_importance,
                                                 &Parfait::CellContainmentChecker::isInCell_c);
    }
    node_statuses = std::move(overset_data->statuses);
    receptors = std::move(overset_data->receptors);

    std::vector<int> ids_of_nodes_to_freeze;
    for (size_t i = 0; i < node_statuses.size(); i++) {
        auto& s = node_statuses[i];
        if (YOGA::InNode != s) ids_of_nodes_to_freeze.push_back(i);
    }
    return ids_of_nodes_to_freeze;
}

std::map<int, std::vector<double>> YogaPlugin::updateReceptorSolutions(
    std::function<void(int, double, double, double, double*)> getter) const {
    int nvar = 5;
    auto framework_getter = [&](int yoga_id, double x, double y, double z, double* solution) {
        getter(framework_cell_ids[yoga_id], x, y, z, solution);
    };
    Tracer::begin("Create inverse receptors");
    auto inverse_receptors = generateInverseReceptors<double>(mp, receptors, mesh);
    Tracer::end("Create inverse receptors");
    Tracer::begin("get solutions from solver");
    std::vector<double> solution_at_nodes(nvar * mesh.nodeCount());
    getUpdatedSolutionsFromSolver(mp, mesh, receptors, inverse_receptors, nvar, framework_getter, solution_at_nodes);
    std::map<int, std::vector<double>> receptor_solutions;
    for (auto& r : receptors) {
        int node_id = r.first;
        for (int i = 0; i < nvar; i++) {
            receptor_solutions[node_id].push_back(solution_at_nodes[nvar * node_id + i]);
        }
    }
    Tracer::end("get solutions from solver");

    return receptor_solutions;
}

std::vector<std::string> YogaPlugin::listFields() const {
    return {"node statuses", "distance", "partition", "boundary conditions", "cell statuses"};
}

std::shared_ptr<FieldInterface> YogaPlugin::field(std::string field_name) const {
    if ("node statuses" == field_name) {
        std::vector<double> status(node_statuses.size());
        for (size_t i = 0; i < status.size(); i++) {
            status[i] = double(node_statuses[i]);
        }
        return std::make_shared<VectorFieldAdapter>(field_name, inf::FieldAttributes::Node(), 1, status);
    } else if ("boundary conditions" == field_name) {
        std::vector<double> bcs(mesh.nodeCount(), YOGA::BoundaryConditions::NotABoundary);
        for (int i = 0; i < mesh.numberOfBoundaryFaces(); i++) {
            auto face = mesh.getNodesInBoundaryFace(i);
            int bc = mesh.getBoundaryCondition(i);
            for (int id : face) {
                bcs[id] = bc;
            }
        }
        std::transform(bcs.begin(), bcs.end(), bcs.begin(), [](double x) { return x - 9000; });
        return std::make_shared<VectorFieldAdapter>(field_name, inf::FieldAttributes::Node(), 1, bcs);
    } else if ("cell statuses" == field_name) {
        std::vector<double> cell_statuses(mesh.numberOfCells() + mesh.numberOfBoundaryFaces(), 0);
        std::vector<int> cell;
        for (int i = 0; i < mesh.numberOfCells(); i++) {
            cell.resize(mesh.numberOfNodesInCell(i));
            mesh.getNodesInCell(i, cell.data());
            int status = determineCellStatusBasedOnNodes(cell, node_statuses);
            int framework_cell_id = framework_cell_ids[i];
            cell_statuses[framework_cell_id] = status;
        }
        return std::make_shared<VectorFieldAdapter>(field_name, inf::FieldAttributes::Cell(), 1, cell_statuses);
    } else if ("partition" == field_name) {
        std::vector<double> partition(mesh.numberOfCells() + mesh.numberOfBoundaryFaces(), mp.Rank());
        return std::make_shared<VectorFieldAdapter>(field_name, inf::FieldAttributes::Cell(), 1, partition);
    } else if ("distance" == field_name) {
        Tracer::begin("build surfaces");
        auto surfaces = YOGA::ParallelSurface::buildSurfaces(mp, mesh);
        Tracer::end("build surfaces");
        auto distanceToWallFromNode = YOGA::DistanceFieldAdapter::getNodeDistances(mesh, surfaces);
        return std::make_shared<VectorFieldAdapter>(
            field_name, inf::FieldAttributes::Node(), 1, distanceToWallFromNode);
    } else {
        throw std::logic_error("Yoga doesn't provide " + field_name);
    }
}

std::vector<YOGA::BoundaryConditions> YogaPlugin::createBoundaryConditionVector(const inf::MeshInterface& mesh,
                                                                                std::string input,
                                                                                int domain_id) {
    auto bc_info = YOGA::BoundaryInfoParser::parse(input)[domain_id];
    std::vector<int> wall_tags = bc_info.solid_wall_tags;
    std::vector<int> interpolation_tags = bc_info.interpolation_tags;
    std::vector<int> x_symmetry_tags = bc_info.x_symmetry_tags;
    std::vector<int> y_symmetry_tags = bc_info.y_symmetry_tags;
    std::vector<int> z_symmetry_tags = bc_info.z_symmetry_tags;
    std::vector<YOGA::BoundaryConditions> boundary_conditions;

    {
        auto tri_to_cell_id = YOGA::MeshInterfaceAdapter::createTriToCellMap(mesh);

        for (int i : tri_to_cell_id) {
            int tag = mesh.cellTag(i);
            if (contains(wall_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::Solid);
            else if (contains(interpolation_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::Interpolation);
            else if (contains(x_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryX);
            else if (contains(y_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryY);
            else if (contains(z_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryZ);
            else
                boundary_conditions.push_back(YOGA::BoundaryConditions::Irrelevant);
        }
    }

    {
        auto quad_to_cell_id = YOGA::MeshInterfaceAdapter::createQuadToCellMap(mesh);

        for (int i : quad_to_cell_id) {
            int tag = mesh.cellTag(i);
            if (contains(wall_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::Solid);
            else if (contains(interpolation_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::Interpolation);
            else if (contains(x_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryX);
            else if (contains(y_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryY);
            else if (contains(z_symmetry_tags, tag))
                boundary_conditions.push_back(YOGA::BoundaryConditions::SymmetryZ);
            else
                boundary_conditions.push_back(YOGA::BoundaryConditions::Irrelevant);
        }
    }

    return boundary_conditions;
}
int YogaPlugin::determineCellStatusBasedOnNodes(const std::vector<int>& cell,
                                                const std::vector<YOGA::NodeStatus>& statuses) const {
    bool has_out = false;
    bool has_receptor = false;
    bool has_orphan = false;
    for (auto id : cell) {
        auto& s = statuses[id];
        if (s == YOGA::NodeStatus::OutNode)
            has_out = true;
        else if (s == YOGA::NodeStatus::FringeNode)
            has_receptor = true;
        else if (s == YOGA::NodeStatus::Orphan)
            has_orphan = true;
    }
    if (has_orphan) return YOGA::NodeStatus::Orphan;
    if (has_receptor) return YOGA::NodeStatus::FringeNode;
    if (has_out) return YOGA::NodeStatus::OutNode;
    return YOGA::NodeStatus::InNode;
}
std::vector<int> YogaPlugin::determineGridIdsForNodes() {
    return YOGA::ComponentGridIdentifier::determineGridIdsForNodes(mp, mesh);
}
void YogaPlugin::verifyDonors() {
    std::vector<double> linear_field(mesh.nodeCount(), 0.0);
    std::vector<double> interpolated_field(mesh.nodeCount(), 0.0);
    for (int i = 0; i < mesh.nodeCount(); i++) linear_field[i] = linearTestFunction(mesh.getNode<double>(i));

    auto inverse_receptors = generateInverseReceptors<double>(mp, receptors, mesh);
    auto get_value = [&](int node_id, double* x) { *x = linear_field[node_id]; };

    WeightBasedInterpolator<double> interpolator(1, inverse_receptors, mesh, &calcWeightsWithFiniteElements);

    YOGA::getUpdatedSolutionsFromInterpolator(
        mp, receptors, inverse_receptors, 1, interpolator, get_value, interpolated_field);

    double max_error = 0.0;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        if (node_statuses[i] == NodeStatus::FringeNode and mesh.nodeOwner(i) == mp.Rank()) {
            double normalize = 100.0 / std::abs(linear_field[i]);
            double error = normalize * std::abs(linear_field[i] - interpolated_field[i]);
            max_error = std::max(error, max_error);
        }
    }
    max_error = mp.ParallelMax(max_error, 0);
    mp_rootprint("Max interpolation error: %lf%%\n", max_error)
}

std::shared_ptr<DomainAssemblerInterface> YogaPluginFactory::createDomainAssembler(MPI_Comm comm,
                                                                                   const MeshInterface& mesh,
                                                                                   int component_id,
                                                                                   std::string json) const {
    MessagePasser mp(comm);
    auto yoga = std::make_shared<YogaPlugin>(mp, mesh, component_id, json);
    return yoga;
}

std::shared_ptr<DomainAssemblerFactory> createDomainAssemblerFactory() { return std::make_shared<YogaPluginFactory>(); }
