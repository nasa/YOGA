#include "ParfaitViz.h"
#include "ParallelTecplotWriter.h"
#include "ParallelVTKWriter.h"
#include "ParallelDataFrameWriter.h"
#include "../shared/SU2IO.h"

#include <parfait/TecplotWriter.h>
#include <parfait/PointWriter.h>
#include <parfait/TetGenWriter.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshGatherer.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/Snap.h>
#include <t-infinity/InfinityToVtk.h>
#include <parfait/TecplotBinaryWriter.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/Extract.h>
#include <parfait/Flatten.h>
#include <parfait/STL.h>

#ifdef HAVE_HO_TECIO
#include "SzPltWriter.h"
#endif

ParfaitViz::ParfaitViz(std::string filename, const std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm)
    : filename(filename),
      mesh(mesh),
      mp(comm),
      owned_global_cell_ids(inf::extractOwnedGlobalCellIds(*mesh)),
      owned_global_node_ids(inf::extractOwnedGlobalNodeIds(*mesh)) {}

std::vector<double> extractOwnedDataAtNodes(inf::FieldInterface& f, inf::MeshInterface& mesh) {
    std::vector<double> field;
    for (int n = 0; n < mesh.nodeCount(); n++) {
        if (mesh.ownedNode(n)) {
            double d;
            f.value(n, &d);
            field.push_back(d);
        }
    }
    return field;
}

std::vector<double> extractOwnedDataAtCells(inf::FieldInterface& f, inf::MeshInterface& mesh) {
    std::vector<double> field;
    std::vector<double> entry(f.blockSize());
    if (f.blockSize() != 1) {
        PARFAIT_WARNING("ParfaitViz assumes all fields are scalars.");
        PARFAIT_WARNING("But it was passed field " + f.name() +
                        " which has number of entries: " + std::to_string(f.blockSize()));
        PARFAIT_WARNING("The first scalar will be written to the file, and the rest will be ignored");
    }
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c)) {
            f.value(c, entry.data());
            field.push_back(entry.front());
        }
    }
    return field;
}

void ParfaitViz::addField(std::shared_ptr<inf::FieldInterface> field) {
    if (field->blockSize() != 1) {
        auto split_fields = inf::FieldTools::split(field);
        for (auto& f : split_fields) {
            addField(f);
        }
    } else {
        if (field->association() == inf::FieldAttributes::Node()) {
            addNodeField(field);
        } else if (field->association() == inf::FieldAttributes::Cell()) {
            addCellField(field);
        } else {
            PARFAIT_WARNING(
                "NoDepVTK add encountered an association that was not NODE or CELL\nAssuming NODE however data may be "
                "corrupted");
            addNodeField(field);
        }
    }
}
void ParfaitViz::addCellField(const std::shared_ptr<inf::FieldInterface>& field) {
    if (mesh->cellCount() != field->size())
        PARFAIT_THROW("Added cell field " + field->name() + " with unexpected number of cells.  Mesh has " +
                      std::to_string(mesh->cellCount()) + ", field has " + std::to_string(field->size()));
    cell_field_ptrs.push_back(field);
}
void ParfaitViz::addNodeField(const std::shared_ptr<inf::FieldInterface>& field) {
    if (mesh->nodeCount() != field->size())
        PARFAIT_THROW("Added node field " + field->name() + " with unexpected number of nodes");
    node_field_ptrs.push_back(field);
}

template <typename GetPoint, typename GetType, typename GetNodes>
inline std::shared_ptr<Parfait::Visualization::Writer> getWriterByFilename(
    std::string filename, long node_count, long cell_count, GetPoint get_point, GetType get_type, GetNodes get_nodes) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "tec") {
        return std::make_shared<Parfait::tecplot::Writer>(
            filename, node_count, cell_count, get_point, get_type, get_nodes);
    } else if (extension == "plt") {
        return std::make_shared<Parfait::tecplot::BinaryWriter>(
            filename, node_count, cell_count, get_point, get_type, get_nodes);
    } else {
        return std::make_shared<Parfait::vtk::Writer>(filename, node_count, cell_count, get_point, get_type, get_nodes);
    }
}

void ParfaitViz::writeUsingSerialWriter() {
    std::map<std::string, std::vector<double>> node_fields;
    std::map<std::string, std::vector<double>> cell_fields;

    for (auto& f : node_field_ptrs) {
        auto node_data = extractOwnedDataAtNodes(*f, *mesh);
        node_fields[f->name()] = mp.GatherByIds(node_data, owned_global_node_ids, root);
    }
    for (auto& f : cell_field_ptrs) {
        auto cell_data = extractOwnedDataAtCells(*f, *mesh);
        cell_fields[f->name()] = mp.GatherByIds(cell_data, owned_global_cell_ids, root);
    }

    std::shared_ptr<inf::MeshInterface> gathered_mesh = inf::gatherMesh(*mesh, mp.getCommunicator(), root);

    if (mp.Rank() == root) {
        auto g2l_cell = inf::GlobalToLocal::buildCell(*gathered_mesh);
        auto get_point = [&](int node_id, double* p) { gathered_mesh->nodeCoordinate(node_id, p); };

        auto get_vtk_cell_type = [&](int global_id) {
            if (g2l_cell.count(global_id) == 0)
                PARFAIT_THROW("Cant get local cell for global: " + std::to_string(global_id) +
                              "\nTotal global cell ids: " + std::to_string(gathered_mesh->cellCount()));
            int cell_id = g2l_cell.at(global_id);
            auto type = gathered_mesh->cellType(cell_id);
            return Parfait::vtk::Writer::CellType(infinityToVtkCellType(type));
        };

        auto get_cell_nodes = [&](int global_id, int* cell_nodes) {
            if (g2l_cell.count(global_id) == 0)
                PARFAIT_THROW("Cant get local cell for global: " + std::to_string(global_id));
            int cell_id = g2l_cell.at(global_id);
            gathered_mesh->cell(cell_id, cell_nodes);
        };

        auto writer = getWriterByFilename(filename,
                                          gathered_mesh->nodeCount(),
                                          gathered_mesh->cellCount(),
                                          get_point,
                                          get_vtk_cell_type,
                                          get_cell_nodes);

        for (auto& pair : cell_fields) {
            auto name = pair.first;
            auto& field_vector = cell_fields.at(name);
            auto get = [&](int cell_id) { return field_vector[cell_id]; };
            writer->addCellField(name, get);
        }

        for (auto& pair : node_fields) {
            auto name = pair.first;
            auto& field_vector = node_fields.at(name);
            auto get = [&](int node_id) { return field_vector[node_id]; };
            writer->addNodeField(name, get);
        }
        writer->visualize();
    }
}

void ParfaitViz::writeUsingParallelTecplotWriter() {
    ParallelTecplotWriter writer(filename, mesh, mp.getCommunicator());

    for (auto& field : node_field_ptrs) {
        writer.addField(field);
    }
    for (auto& field : cell_field_ptrs) {
        writer.addField(field);
    }
    writer.write();
}

void ParfaitViz::writeUsingSzPlt() {
#ifdef HAVE_HO_TECIO

    auto gathered_mesh = inf::gatherMesh(*mesh, mp.getCommunicator(), root);
    inf::SzWriter writer(filename, *gathered_mesh);

    for (auto& f : node_field_ptrs) {
        auto node_data = extractOwnedDataAtNodes(*f, *mesh);
        node_data = mp.GatherByIds(node_data, owned_global_node_ids, root);
        writer.addNodeField(
            std::make_shared<inf::VectorFieldAdapter>(f->name(), inf::FieldAttributes::Node(), node_data));
    }
    for (auto& f : cell_field_ptrs) {
        auto cell_data = extractOwnedDataAtCells(*f, *mesh);
        cell_data = mp.GatherByIds(cell_data, owned_global_cell_ids, root);
        writer.addCellField(
            std::make_shared<inf::VectorFieldAdapter>(f->name(), inf::FieldAttributes::Cell(), cell_data));
    }

    writer.write();

#else
    PARFAIT_THROW("Cannot write .szlplt file.  TecIO not found.");
#endif
}

void ParfaitViz::writeUsingParallelVTKWriter() {
    ParallelVTKWriter writer(filename, mesh, mp.getCommunicator());

    bool added_cell_tags = false;
    for (auto& field : node_field_ptrs) {
        writer.addField(field);
    }
    for (auto& field : cell_field_ptrs) {
        writer.addField(field);
        if (field->name() == "Cell tag") added_cell_tags = true;
    }
    if (not added_cell_tags) {
        std::vector<double> cell_tag(mesh->cellCount());
        for (int i = 0; i < mesh->cellCount(); i++) {
            cell_tag[i] = mesh->cellTag(i);
        }
        auto tags = std::make_shared<inf::VectorFieldAdapter>("Cell tag", inf::FieldAttributes::Cell(), 1, cell_tag);
        writer.addField(tags);
    }
    writer.write();
}

void ParfaitViz::writeUsingParallelUgridWriter() { inf::ParallelUgridExporter::write(filename, *mesh, mp); }

void ParfaitViz::writeSnapDatabase() {
    inf::Snap snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    for (auto f : node_field_ptrs) {
        snap.add(f);
    }
    for (auto f : cell_field_ptrs) {
        snap.add(f);
    }
    int num_fields = node_field_ptrs.size() + cell_field_ptrs.size();
    if (num_fields > 0) snap.writeFile(filename);
}

void ParfaitViz::visualize() {
    renameFileIfAsciiTecplotRequested();
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "snap") {
        writeSnapDatabase();
    } else if (extension == "ugrid") {
        if (areThereFieldsOtherThanCellTag()) {
            std::string first_ext, last_ext;
            Parfait::StringTools::parseUgridFilename(filename, first_ext, last_ext);
            filename += ".snap";
            // Since we add Cell Tag no matter what
            // We don't want to write a snap file if that's all that's in the file.
            // Cell tags are in the ugrid file for surface cells anyway.
            writeSnapDatabase();
            filename = Parfait::StringTools::stripExtension(filename);
            filename = Parfait::StringTools::combineUgridFilename(filename, first_ext, last_ext);
        }
        writeUsingParallelUgridWriter();

    } else if (extension == "su2") {
        if (areThereFieldsOtherThanCellTag()) {
            std::string first_ext, last_ext;
            Parfait::StringTools::parseUgridFilename(filename, first_ext, last_ext);
            filename += ".snap";
            // Since we add Cell Tag no matter what
            // We don't want to write a snap file if that's all that's in the file.
            // Cell tags are in the ugrid file for surface cells anyway.
            writeSnapDatabase();
            filename = Parfait::StringTools::stripExtension(filename);
            filename += ".su2";
        }
        writeSU2();
    } else if (extension == "plt") {
        writeUsingParallelTecplotWriter();
    } else if (extension == "szplt") {
        writeUsingSzPlt();
    } else if (extension == "svtk" or extension == "tec") {
        writeUsingSerialWriter();
    } else if (extension == "csv" or extension == "dat") {
        writeUsingDataFrameWriter();
    } else if (extension == "stl") {
        writeSTL();
    } else if (extension == "smesh" or extension == "poly") {
        writeTetGen();
    } else if (extension == "3D") {
        writePoints3D();
    } else {
        writeUsingParallelVTKWriter();
    }
}
void ParfaitViz::renameFileIfAsciiTecplotRequested() {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "tec") {
        mp_rootprint("Warning! The file format requested is the ascii .tec file.\n");
        mp_rootprint("Warning! We assume you want binary Tecplot format .plt\n");
        filename = Parfait::StringTools::stripExtension(filename, "tec");
        filename += ".plt";
    }
    if (extension == "tec-ascii") {
        mp_rootprint("Detected that the ascii tecplot format was requested\n");
        filename = Parfait::StringTools::stripExtension(filename, "tec-ascii");
        filename += ".tec";
    }
}
void ParfaitViz::writeSTL() {
    auto facets = Parfait::flatten(mp.Gather(inf::extractOwned2DFacets(mp, *mesh), 0));
    if (mp.Rank() == 0) Parfait::STL::write(facets, filename);
}
void ParfaitViz::writeSU2() {
    auto gathered_mesh = inf::gatherMesh(*mesh, mp.getCommunicator(), 0);
    if (mp.Rank() == 0) {
        SU2Writer writer(filename, gathered_mesh);
        writer.write();
    }
}
void ParfaitViz::writePoints3D() {
    auto points = inf::extractOwnedPoints(*mesh);
    Parfait::PointWriter::write(mp, filename, points);
}
void ParfaitViz::writeUsingDataFrameWriter() {
    ParallelDataFrameWriter writer(filename, mesh, mp);
    for (auto f : node_field_ptrs) {
        writer.addField(f);
    }
    for (auto f : cell_field_ptrs) {
        writer.addField(f);
    }
    writer.write();
}
void ParfaitViz::writeTetGen() {
    auto filter = inf::FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    std::shared_ptr<inf::MeshInterface> gathered_mesh = inf::gatherMesh(*filter->getMesh(), mp.getCommunicator(), root);
    if (mp.Rank() == root) {
        int num_points = gathered_mesh->nodeCount();
        auto getPoint = [&](int i) {
            Parfait::Point<double> p;
            gathered_mesh->nodeCoordinate(i, p.data());
            return p;
        };
        if (gathered_mesh->cellCount(inf::MeshInterface::TRI_3) != gathered_mesh->cellCount()) {
            PARFAIT_WARNING("Refusing to write tetgen surface mesh containing more than just TRI_3");
        }
        int num_tris = gathered_mesh->cellCount();
        std::array<int, 3> tri;
        auto fillTri = [&](int i, long* t) {
            gathered_mesh->cell(i, tri.data());
            t[0] = tri[0];
            t[1] = tri[1];
            t[2] = tri[2];
        };

        auto getTag = [&](int t) { return gathered_mesh->cellTag(t); };

        int num_quads = 0;
        auto fillQuad = [](int t, long* q) { PARFAIT_THROW("You asked for a quad when there is no quad") };
        auto getQuadTag = [](int t) -> int { PARFAIT_THROW("You asked for a quad when there is no quad") };
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension == "smesh") {
            Parfait::TetGen::writeSmesh(
                filename, getPoint, num_points, fillTri, getTag, num_tris, fillQuad, getQuadTag, num_quads);
        } else if (extension == "poly") {
            Parfait::TetGen::writePoly(
                filename, getPoint, num_points, fillTri, getTag, num_tris, fillQuad, getQuadTag, num_quads);
        }
    }
}
bool ParfaitViz::areThereFieldsOtherThanCellTag() {
    if (node_field_ptrs.size() > 0) return true;
    if (cell_field_ptrs.size() > 1) return true;
    if (cell_field_ptrs.size() == 0) return false;
    if (Parfait::StringTools::tolower(cell_field_ptrs.front()->name()) != "cell tag") return true;
    return false;
}

std::shared_ptr<inf::VizInterface> ParfaitVizFactory::create(std::string name,
                                                             std::shared_ptr<inf::MeshInterface> mesh,
                                                             MPI_Comm comm) const {
    return std::make_shared<ParfaitViz>(name, mesh, comm);
}

std::shared_ptr<inf::VizFactory> createVizFactory() { return std::make_shared<ParfaitVizFactory>(); }
