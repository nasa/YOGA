#include "PybindIncludes.h"
#include "t-infinity/MeshShuffle.h"

#include <t-infinity/MeshInterface.h>
#include <t-infinity/AddHalo.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/MeshMover.h>
#include <t-infinity/MeshExploder.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/MeshShard.h>
#include <t-infinity/MeshGatherer.h>
#include <t-infinity/BetterDistanceCalculator.h>
#include <t-infinity/DistanceCalculatorInterface.h>
#include <t-infinity/BandwidthReducer.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/RepartitionerInterface.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/Extract.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/StructuredMeshLoader.h>
#include <t-infinity/StructuredSubMesh.h>
#include <t-infinity/StructuredToUnstructuredMeshAdapter.h>
#include <t-infinity/StructuredMeshConnectivity.h>
#include <t-infinity/StructuredMeshSequencing.h>
#include <t-infinity/GhostSyncer.h>
#include <t-infinity/SMesh.h>

PYBIND11_MAKE_OPAQUE(inf::MeshConnectivity)

using namespace inf;
namespace py = pybind11;

void PythonMeshBindings(py::module& m) {
    typedef std::shared_ptr<MeshInterface> Mesh;
    m.def("scaleMesh", &MeshMover::scale);
    m.def("moveMesh", &MeshMover::move);
    m.def("explodeMesh", &explodeMesh);
    m.def("checkMesh", &MeshSanityChecker::checkAll);
    m.def("reduceBandwithViaReverseCutthillMckee", &BandwidthReducer::reoderNodes);
    m.def("addHalo",
          [](MessagePasser mp, Mesh mesh) { return addHaloNodes(mp.getCommunicator(), *mesh); });
    m.def("repartitionByVolumeCells", [](MessagePasser mp, Mesh mesh) -> Mesh {
        return MeshShuffle::repartitionByVolumeCells(mp, *mesh);
    });
    m.def("repartitionCells",
          [](MessagePasser mp, Mesh mesh, std::vector<double> cell_costs) -> Mesh {
              auto new_cell_owners = MeshShuffle::repartitionCells(mp, *mesh, cell_costs);
              return MeshShuffle::shuffleCells(mp, *mesh, new_cell_owners);
          });
    m.def("repartitionNodes",
          [](MessagePasser mp, Mesh mesh, std::vector<double> node_costs) -> Mesh {
              auto new_node_owners = MeshShuffle::repartitionNodes(mp, *mesh, node_costs);
              return MeshShuffle::shuffleNodes(mp, *mesh, new_node_owners);
          });
    m.def("gatherMesh", [](MessagePasser mp, Mesh mesh, int root_rank) -> Mesh {
        return gatherMesh(*mesh, mp.getCommunicator(), root_rank);
    });
    m.def("exportUgrid", [](std::string name, Mesh mesh, MessagePasser mp) {
        ParallelUgridExporter::write(name, *mesh, mp.getCommunicator());
    });
    m.def("calculateDistance", [](MessagePasser mp, Mesh mesh, std::vector<int>& tags) {
        PARFAIT_ASSERT(not is2D(mp, *mesh), "Distance calculator only supports 3D meshs");
        std::set<int> tags_set(tags.begin(), tags.end());
        std::vector<double> wall_distance;
        std::tie(wall_distance, std::ignore, std::ignore) =
            calcDistanceToNodes(mp, *mesh, tags_set);
        return wall_distance;
    });
    m.def("extractAllSurfaceTags", [](MessagePasser mp, Mesh mesh) {
        auto tags = extractAllTagsWithDimensionality(mp, *mesh, 2);
        return std::vector<int>(tags.begin(), tags.end());
    });
    m.def("addMeshTagNames", [](Mesh mesh, std::map<int, std::string> tag_names) -> Mesh {
        auto tinf_mesh = std::dynamic_pointer_cast<TinfMesh>(mesh);
        if (not tinf_mesh) tinf_mesh = std::make_shared<TinfMesh>(mesh);
        for (const auto& t : tag_names) tinf_mesh->setTagName(t.first, t.second);
        return tinf_mesh;
    });
    m.def("copyMesh", [](Mesh mesh) -> Mesh { return std::make_shared<TinfMesh>(mesh); });

    typedef std::shared_ptr<MeshLoader> PyMeshLoader;
    py::class_<MeshLoader, PyMeshLoader>(m, "PyMeshLoader")
        .def(py::init(
            [](std::string directory, std::string name) { return getMeshLoader(directory, name); }))
        .def("load",
             [](PyMeshLoader self, std::string file, MessagePasser mp) {
                 return self->load(mp.getCommunicator(), file);
             })
        .def("unloadPlugin", [](PyMeshLoader self) { self.reset(); });

    py::class_<MeshInterface, Mesh> PyMesh(m, "Mesh");
    PyMesh.def("nodeCount", &MeshInterface::nodeCount)
        .def("cellCount", [](Mesh self) { return self->cellCount(); })
        .def("cellSize", &MeshInterface::cellLength)
        .def("cellNodes",
             [](Mesh self, int id) {
                 std::vector<int> node_ids;
                 self->cell(id, node_ids);
                 return node_ids;
             })
        .def(
            "cell",
            [](Mesh self, int id, py::array_t<int>& x) {
                auto r = x.mutable_unchecked<1>();
                self->cell(id, r.mutable_data(0));
            },
            py::arg("id"),
            py::arg("x").noconvert())
        .def("cellTag", &MeshInterface::cellTag)
        .def("tagName", &MeshInterface::tagName)
        .def("cellType", &MeshInterface::cellType)
        .def("cellOwner", &MeshInterface::cellOwner)
        .def("ownedCell", &MeshInterface::ownedCell)
        .def("is2DCellType", &MeshInterface::is2DCellType)
        .def("isSurfaceCell", &MeshInterface::is2DCell)
        .def("nodeOwner", &MeshInterface::nodeOwner)
        .def("ownedNode", &MeshInterface::ownedNode)
        .def("partitionId", &MeshInterface::partitionId)
        .def("globalNodeId", &MeshInterface::globalNodeId)
        .def("nodeCoordinate",
             [](Mesh self, int id) {
                 std::array<double, 3> p;
                 self->nodeCoordinate(id, p.data());
                 return p;
             })
        .def("surfaceCellArea",
             [](Mesh self, int cell_id) {
                 auto area = self->is2DCell(cell_id) ? Cell(*self, cell_id).faceAreaNormal(0)
                                                     : Parfait::Point<double>{};
                 return py::make_tuple(area[0], area[1], area[2]);
             })
        .def("globalNodeCount",
             [](Mesh self, MessagePasser mp) { return globalNodeCount(mp, *self); })
        .def("globalCellCount",
             [](Mesh self, MessagePasser mp) { return globalCellCount(mp, *self); })
        .def("globalCellTypeCount",
             [](Mesh self, MessagePasser mp, MeshInterface::CellType cell_type) {
                 return globalCellCount(mp, *self, cell_type);
             })
        .def("is2D", [](Mesh self, MessagePasser mp) { return is2D(mp, *self); })
        .def("allSurfaceTags",
             [](Mesh self, MessagePasser mp) {
                 return extractAllTagsWithDimensionality(
                     mp, *self, maxCellDimensionality(mp, *self) - 1);
             })
        .def("buildNodeToNode", [](Mesh self) { return NodeToNode::build(*self); })
        .def("buildNodeToNodeOnSurfaceOnly",
             [](Mesh self) { return NodeToNode::buildSurfaceOnly(*self); })
        .def("getNodesOnTags", [](Mesh self, MessagePasser mp, std::set<int> tags) {
            auto surface_dimensionality = maxCellDimensionality(mp, *self) - 1;
            std::set<int> nodes_in_tags;
            for (int tag : tags) {
                auto n = extractNodeIdsViaDimensionalityAndTag(*self, tag, surface_dimensionality);
                nodes_in_tags.insert(n.begin(), n.end());
            }
            return nodes_in_tags;
        });

    py::enum_<MeshInterface::CellType> PyMeshCellType(PyMesh, "Kind");
#define addMeshEnum(name) PyMeshCellType.value(#name, MeshInterface::CellType::name)
    addMeshEnum(BAR_2);
    addMeshEnum(TRI_3);
    addMeshEnum(QUAD_4);
    addMeshEnum(TETRA_4);
    addMeshEnum(PYRA_5);
    addMeshEnum(PENTA_6);
    addMeshEnum(HEXA_8);
    PyMeshCellType.export_values();

    typedef std::shared_ptr<StructuredMesh> PyStructuredMesh;
    py::class_<StructuredMesh, PyStructuredMesh> PySMesh(m, "StructuredMesh");
    PySMesh
        .def("blockNodeDimensions",
             [](PyStructuredMesh self, int block_id) {
                 return self->getBlock(block_id).nodeDimensions();
             })
        .def("blockIds", &StructuredMesh::blockIds)
        .def("doubleBlock", [](PyStructuredMesh self, int block_id_to_double, int axis_to_double) {
            auto smesh = std::dynamic_pointer_cast<inf::SMesh>(self);
            PARFAIT_ASSERT(smesh, "doubling only supported by inf::SMesh");
            smesh->blocks[block_id_to_double] = inf::doubleStructuredBlock(
                smesh->getBlock(block_id_to_double), static_cast<BlockAxis>(axis_to_double));
        });

    py::enum_<BlockSide> PyBlockSide(m, "BlockSide");
#define addBlockSide(side) PyBlockSide.value(#side, side)
    addBlockSide(IMIN);
    addBlockSide(JMIN);
    addBlockSide(KMIN);
    addBlockSide(IMAX);
    addBlockSide(JMAX);
    addBlockSide(KMAX);
    PyBlockSide.export_values();

    typedef std::shared_ptr<StructuredTinfMesh> PyStructuredTinfMesh;
    py::class_<StructuredTinfMesh, PyStructuredTinfMesh>(m, "StructuredTinfMesh", PyMesh)
        .def("blockIds", &StructuredTinfMesh::blockIds)
        .def("cellId", &StructuredTinfMesh::cellId)
        .def("nodeId", &StructuredTinfMesh::nodeId)
        .def("shallowCopyToStructuredMesh",
             [](PyStructuredTinfMesh self) { return self->shallowCopyToStructuredMesh(); })
        .def("blockNodeDimensions",
             [](PyStructuredTinfMesh self, int block_id) {
                 return self->getBlock(block_id).nodeDimensions();
             })
        .def("blockCellDimensions", [](PyStructuredTinfMesh self, int block_id) {
            return self->getBlock(block_id).nodeDimensions();
        });

    typedef std::shared_ptr<StructuredSubMesh> PyStructuredSubMesh;
    py::class_<StructuredSubMesh, PyStructuredSubMesh>(m, "StructuredSubMesh", PySMesh)
        .def("previousBlockIJK", [](PyStructuredSubMesh self, int block_id, int i, int j, int k) {
            auto previous = self->getBlock(block_id).previousNodeBlockIJK(i, j, k);
            return py::make_tuple(previous.block_id, previous.i, previous.j, previous.k);
        });

    typedef std::shared_ptr<StructuredMeshLoader> PyStructuredMeshLoader;
    py::class_<StructuredMeshLoader, PyStructuredMeshLoader>(m, "StructuredMeshLoader")
        .def(py::init([](std::string directory, std::string name) {
            return getStructuredMeshLoader(directory, name);
        }))
        .def("load", [](PyStructuredMeshLoader self, std::string file, MessagePasser mp) {
            return self->load(mp.getCommunicator(), file);
        });

    py::bind_map<MeshConnectivity>(m, "StructuredMeshConnectivity");
    m.def("buildStructuredMeshConnectivityFromLAURABoundaryConditions",
          [](std::map<int, std::array<int, 6>> laura_boundary_conditions) {
              MeshConnectivity connectivity;
              for (const auto& p : laura_boundary_conditions)
                  connectivity[p.first] =
                      buildBlockConnectivityFromLAURABoundaryConditions(p.second);
              return connectivity;
          });
    m.def("buildStructuredMeshConnectivityFromLAURABoundaryConditions",
          &buildMeshConnectivityFromLAURABoundaryConditions);

    m.def("buildStructuredMeshConnectivity",
          [](MessagePasser mp, PyStructuredMesh structured_mesh) {
              return buildMeshBlockConnectivity(mp, *structured_mesh);
          });

    m.def("convertStructuredToUnstructuredMesh",
          [](MessagePasser mp, PyStructuredMesh structured_mesh, MeshConnectivity connectivity) {
              return convertStructuredMeshToUnstructuredMesh(mp, *structured_mesh, connectivity);
          });

    typedef std::shared_ptr<RepartitionerInterface> PyMeshRepartitioner;
    py::class_<RepartitionerInterface, PyMeshRepartitioner>(m, "MeshRepartitioner")
        .def(py::init([](std::string directory, std::string name) {
            return getRepartitioner(directory, name);
        }))
        .def("repartition", [](PyMeshRepartitioner self, MessagePasser mp, Mesh mesh) {
            return self->repartition(mp.getCommunicator(), *mesh);
        });

    m.def("buildCartesianMesh",
          [](MessagePasser mp, int ncells_x, int ncells_y, int ncells_z) -> Mesh {
              return inf::CartMesh::create(mp, ncells_x, ncells_y, ncells_z);
          });
    m.def("buildCartesianMesh", [](MessagePasser mp, int ncells_x, int ncells_y) -> Mesh {
        return inf::CartMesh::create2D(mp, ncells_x, ncells_y);
    });
    m.def("buildSphereMesh",
          [](MessagePasser mp, int ncells_x, int ncells_y, int ncells_z) -> Mesh {
              auto sphere = mp.Rank() == 0
                                ? CartMesh::createSphere(ncells_x, ncells_y, ncells_z)
                                : std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());
              return inf::MeshShuffle::repartitionByVolumeCells(mp, *sphere);
          });

    typedef std::shared_ptr<MeshShard> PyMeshShard;
    py::class_<MeshShard, PyMeshShard>(m, "MeshShard")
        .def(py::init(
            [](std::string directory, std::string name) { return getMeshShard(directory, name); }))
        .def("shard", [](PyMeshShard self, MessagePasser mp, Mesh mesh) {
            return self->shard(mp.getCommunicator(), *mesh);
        });

    typedef std::shared_ptr<DistanceCalculatorInterface> PyDistanceCalculator;
    py::class_<DistanceCalculatorInterface, PyDistanceCalculator>(m, "DistanceCalculator")
        .def(py::init([](std::string directory, std::string name) {
            return getDistanceCalculator(directory, name);
        }))
        .def("calculateDistanceToNodes",
             [](PyDistanceCalculator self,
                MessagePasser mp,
                std::set<int> tags,
                std::shared_ptr<MeshInterface> mesh) {
                 return self->calculateDistanceToNodes(mp, tags, *mesh);
             });

    m.def("syncNodeCoordinates", [](MessagePasser mp, Mesh mesh) {
        auto tinf_mesh = std::dynamic_pointer_cast<TinfMesh>(mesh);
        PARFAIT_ASSERT(tinf_mesh, "syncNodeCoordinates requires a TinfMesh.")
        GhostSyncer syncer(mp, tinf_mesh);
        syncer.syncNodes(tinf_mesh->mesh.points);
    });
}
