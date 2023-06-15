#include "SerialPreProcessor.h"
#include <t-infinity/SurfaceElementWinding.h>
#include <t-infinity/SurfaceNeighbors.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/MeshSanityChecker.h>
#include "../shared/ReaderFactory.h"

std::shared_ptr<inf::MeshInterface> SerialPreProcessor::load(MPI_Comm comm, std::string filename) const {
    MessagePasser mp(comm);
    auto reader = ReaderFactory::getReaderByFilename(filename);

    inf::TinfMeshData mesh_data;

    int num_nodes = reader->nodeCount();
    Tracer::begin("read coordinates");
    mesh_data.points = reader->readCoords(0, num_nodes);
    Tracer::end("read coordinates");
    mesh_data.node_owner.resize(num_nodes, 0);
    mesh_data.global_node_id.resize(num_nodes);
    for (long i = 0; i < num_nodes; i++) {
        mesh_data.global_node_id[i] = i;
    }
    auto cell_types = reader->cellTypes();

    long next_global_cell_id = 0;
    for (auto type : cell_types) {
        Tracer::begin("reading " + inf::MeshInterface::cellTypeString(type));
        int num_cells = reader->cellCount(type);
        mesh_data.cell_owner[type].resize(num_cells, 0);
        mesh_data.global_cell_id[type].resize(num_cells);
        mesh_data.cell_tags[type] = reader->readCellTags(type, 0, num_cells);
        auto cells_long = reader->readCells(type, 0, num_cells);
        mesh_data.cells[type].resize(cells_long.size());
        for (unsigned long i = 0; i < cells_long.size(); i++) {
            mesh_data.cells[type][i] = cells_long[i];
        }
        for (long i = 0; i < num_cells; i++) {
            mesh_data.global_cell_id[type][i] = next_global_cell_id++;
        }
        Tracer::end("reading " + inf::MeshInterface::cellTypeString(type));
    }

    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, 0);

    if (not inf::is2D(mp, *mesh)) {
        Tracer::begin("winding surface elements out");
        auto surface_neighbors = SurfaceNeighbors::buildSurfaceNeighbors(mesh);
        SurfaceElementWinding::windAllSurfaceElementsOut(mp, *mesh.get(), surface_neighbors);
        Tracer::end("winding surface elements out");
    }

    return mesh;
}

std::shared_ptr<inf::MeshLoader> createPreProcessor() { return std::make_shared<SerialPreProcessor>(); }
