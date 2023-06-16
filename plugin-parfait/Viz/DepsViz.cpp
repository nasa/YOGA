#include "DepsViz.h"
#include <Tracer.h>
#include <parfait/CellWindingConverters.h>
#include <vtkBiQuadraticQuadraticWedge.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkIntArray.h>
#include <vtkLine.h>
#include <vtkLongArray.h>
#include <vtkPointData.h>
#include <vtkPyramid.h>
#include <vtkQuad.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticWedge.h>
#include <vtkSmartPointer.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertex.h>
#include <vtkWedge.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <array>
#include <map>
#include "InfinityToVtk.h"
#include "VtkHacking.h"

using namespace inf;

std::shared_ptr<VizInterface> HOVizFactory::createViz(std::string name,
                                                      std::shared_ptr<MeshInterface> mesh,
                                                      MPI_Comm comm) const {
    return std::make_shared<DepsViz>(name, mesh, comm);
}

DepsViz::DepsViz(std::string filename, const std::shared_ptr<MeshInterface> mesh, MPI_Comm comm)
    : filename_base(filename), mesh(mesh), mp(comm) {}

vtkSmartPointer<vtkIntArray> createVtkArrayPointer(int32_t i) { return vtkSmartPointer<vtkIntArray>::New(); }

vtkSmartPointer<vtkLongArray> createVtkArrayPointer(int64_t l) { return vtkSmartPointer<vtkLongArray>::New(); }

vtkSmartPointer<vtkFloatArray> createVtkArrayPointer(float f) { return vtkSmartPointer<vtkFloatArray>::New(); }

vtkSmartPointer<vtkDoubleArray> createVtkArrayPointer(double d) { return vtkSmartPointer<vtkDoubleArray>::New(); }

template <typename T>
void DepsViz::addNodeField_byType(std::shared_ptr<FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    auto vtk_data_array = createVtkArrayPointer(T());

    int length = f->blockSize();
    vtk_data_array->SetNumberOfComponents(f->blockSize());
    vtk_data_array->SetName(f->name().c_str());
    std::vector<T> d(f->blockSize());
    for (int node_id = 0; node_id < grid->GetNumberOfPoints(); node_id++) {
        f->value(node_id, d.data());
        for (int j = 0; j < length; j++) {
            vtk_data_array->InsertNextValue(d[j]);
        }
    }

    grid->GetPointData()->AddArray(vtk_data_array);
}

void DepsViz::addCellTags(vtkSmartPointer<vtkUnstructuredGrid> grid) {
    auto vtk_data_array = createVtkArrayPointer(int32_t());

    vtk_data_array->SetNumberOfComponents(1);
    vtk_data_array->SetName("cell-tag");
    for (int cell_id = 0; cell_id < grid->GetNumberOfCells(); cell_id++) {
        int32_t tag = mesh->cellTag(cell_id);
        vtk_data_array->InsertNextValue(tag);
    }

    grid->GetCellData()->AddArray(vtk_data_array);
}

template <typename T>
void DepsViz::addCellField_byType(std::shared_ptr<FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    auto vtk_data_array = createVtkArrayPointer(T());

    int length = f->blockSize();
    vtk_data_array->SetNumberOfComponents(f->blockSize());
    vtk_data_array->SetName(f->name().c_str());
    std::vector<T> d(f->blockSize());
    for (int cell_id = 0; cell_id < grid->GetNumberOfCells(); cell_id++) {
        f->value(cell_id, d.data());
        for (int j = 0; j < length; j++) {
            vtk_data_array->InsertNextValue(d[j]);
        }
    }

    grid->GetCellData()->AddArray(vtk_data_array);
}

void DepsViz::addNodeField(std::shared_ptr<FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    using Type = inf::FieldInterface::DataType;
    auto type = f->entryType();
    switch (type) {
        case Type::FLOAT32:
            addNodeField_byType<float>(f, grid);
            break;
        case Type::FLOAT64:
            addNodeField_byType<double>(f, grid);
            break;
        case Type::INT32:
            addNodeField_byType<int32_t>(f, grid);
            break;
        case Type::INT64:
            addNodeField_byType<int64_t>(f, grid);
            break;
        default:
            throw std::logic_error(
                "ParfaitViz only implementes FLOAT32, FLOAT64, INT32, INT64, as "
                "field types");
    }
}

void DepsViz::addCellField(std::shared_ptr<FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    auto type = f->entryType();
    using Type = inf::FieldInterface::DataType;
    switch (type) {
        case Type::FLOAT32:
            addCellField_byType<float>(f, grid);
            break;
        case Type::FLOAT64:
            addCellField_byType<double>(f, grid);
            break;
        case Type::INT32:
            addCellField_byType<int32_t>(f, grid);
            break;
        case Type::INT64:
            addCellField_byType<int64_t>(f, grid);
            break;
        default:
            throw std::logic_error(
                "ParfaitViz only implementes FLOAT32, FLOAT64, INT32, INT64, as "
                "field types");
    }
}

void DepsViz::visualize() {
    int rank = mp.Rank();
    int nproc = mp.NumberOfProcesses();

    auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    copyGridToVtk(mesh, grid);
    addNodeFields(grid);
    addCellFields(grid);
    addCellTags(grid);
    int i_have_cells;
    if (mesh->cellCount() == 0)
        i_have_cells = 0;
    else
        i_have_cells = 1;
    std::vector<int> does_rank_have_cells;
    mp.Gather(i_have_cells, does_rank_have_cells);
    int number_of_non_empty_partitions = 0;
    for (auto i : does_rank_have_cells) number_of_non_empty_partitions += i;

    int my_piece_number = 0;
    for (int i = 0; i < rank; i++) {
        my_piece_number += does_rank_have_cells[i];
    }

    auto total_count = mp.ParallelSum(mesh->cellCount());
    if (mp.Rank() == 0) printf("Parfait visualizing %d cells to <%s>.\n", total_count, filename_base.c_str());

    if (i_have_cells) {
        auto communicator = vtkSmartPointer<vtkHackCommunicator>::New();
        auto controller = vtkSmartPointer<vtkHackController>::New();
        controller->SetCommunicator(communicator);

        controller->SetNumberOfProcesses(nproc);
        controller->SetRank(rank);

        vtkSmartPointer<vtkXMLPUnstructuredGridWriter> pwriter = vtkSmartPointer<vtkXMLPUnstructuredGridWriter>::New();
        pwriter->SetController(controller);
        std::string parallel_filename = filename_base + ".pvtu";
        pwriter->SetFileName(parallel_filename.c_str());
        pwriter->SetNumberOfPieces(number_of_non_empty_partitions);
        pwriter->SetStartPiece(my_piece_number);
        pwriter->SetEndPiece(my_piece_number);
        pwriter->SetInputData(grid);
        pwriter->Write();
    }
}

void DepsViz::copyGridToVtk(const std::shared_ptr<MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    setCells(mesh, grid);
    setPoints(mesh, grid);
}

void DepsViz::setCells(const std::shared_ptr<MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    std::vector<int> cell_types = mapToVtkCellTypes(mesh);

    for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
        auto type = mesh->cellType(cell_id);
        vtkSmartPointer<vtkCell> vtk_cell = extractVtkCell(mesh, type, cell_id);
        cells->InsertNextCell(vtk_cell);
    }
    grid->SetCells(cell_types.data(), cells);
}

std::vector<int> DepsViz::mapToVtkCellTypes(const std::shared_ptr<MeshInterface>& mesh) {
    std::vector<int> cell_types;
    for (int cell_id = 0; cell_id < mesh->cellCount(); cell_id++) {
        auto type = mesh->cellType(cell_id);
        int vtk_type = infinityToVtkCellType(type);
        cell_types.push_back(vtk_type);
    }
    return cell_types;
}

vtkSmartPointer<vtkCell> allocateVtkCellFromInfinityType(MeshInterface::CellType type) {
    vtkSmartPointer<vtkCell> vtk_cell;
    switch (type) {
        case MeshInterface::NODE:
            vtk_cell = vtkSmartPointer<vtkVertex>::New();
            break;
        case MeshInterface::BAR_2:
            vtk_cell = vtkSmartPointer<vtkLine>::New();
            break;
        case MeshInterface::TRI_3:
            vtk_cell = vtkSmartPointer<vtkTriangle>::New();
            break;
        case MeshInterface::TRI_6:
            vtk_cell = vtkSmartPointer<vtkQuadraticTriangle>::New();
            break;
        case MeshInterface::QUAD_4:
            vtk_cell = vtkSmartPointer<vtkQuad>::New();
            break;
        case MeshInterface::QUAD_8:
            vtk_cell = vtkSmartPointer<vtkQuadraticQuad>::New();
            break;
        case MeshInterface::TETRA_4:
            vtk_cell = vtkSmartPointer<vtkTetra>::New();
            break;
        case MeshInterface::TETRA_10:
            vtk_cell = vtkSmartPointer<vtkQuadraticTetra>::New();
            break;
        case MeshInterface::PYRA_5:
            vtk_cell = vtkSmartPointer<vtkPyramid>::New();
            break;
        case MeshInterface::PENTA_6:
            vtk_cell = vtkSmartPointer<vtkWedge>::New();
            break;
        case MeshInterface::PENTA_15:
            vtk_cell = vtkSmartPointer<vtkQuadraticWedge>::New();
            break;
        case MeshInterface::PENTA_18:
            vtk_cell = vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();
            break;
        case MeshInterface::HEXA_8:
            vtk_cell = vtkSmartPointer<vtkHexahedron>::New();
            break;
        default:
            throw std::logic_error("VTK type related to mesh type not supported. TYPE: " +
                                   MeshInterface::cellTypeString(type));
    }
    return vtk_cell;
}

void mapFromCGNSToVtk(MeshInterface::CellType type, int* cell) {
    switch (type) {
        case MeshInterface::NODE:
        case MeshInterface::BAR_2:
        case MeshInterface::TRI_3:
        case MeshInterface::TRI_6:
        case MeshInterface::QUAD_4:
        case MeshInterface::PYRA_13:
        case MeshInterface::TETRA_10:
            return;
        case MeshInterface::TETRA_4:
            Parfait::CGNSToVtk::convertTet(cell);
            return;
        case MeshInterface::PYRA_5:
            Parfait::CGNSToVtk::convertPyramid(cell);
            return;
        case MeshInterface::PENTA_6:
            Parfait::CGNSToVtk::convertPrism(cell);
            return;
        case MeshInterface::PENTA_15:
            Parfait::CGNSToVtk::convertPenta_15(cell);
            return;
        case MeshInterface::PENTA_18:
            Parfait::CGNSToVtk::convertPenta_18(cell);
            return;
        case MeshInterface::HEXA_8:
            Parfait::CGNSToVtk::convertHex(cell);
            return;
        default:
            throw std::logic_error("VTK type related to mesh type not supported. TYPE: " +
                                   MeshInterface::cellTypeString(type));
    }
}

vtkSmartPointer<vtkCell> DepsViz::extractVtkCell(const std::shared_ptr<MeshInterface>& mesh,
                                                 MeshInterface::CellType type,
                                                 int cell_id) {
    vtkSmartPointer<vtkCell> vtk_cell = allocateVtkCellFromInfinityType(type);

    int cell_length = MeshInterface::cellTypeLength(type);
    std::vector<int> cell(static_cast<unsigned long>(cell_length));
    mesh->cell(cell_id, cell.data());
    mapFromCGNSToVtk(type, cell.data());

    for (int i = 0; i < cell_length; i++) {
        vtk_cell->GetPointIds()->SetId(i, cell[i]);
    }

    return vtk_cell;
}

void DepsViz::setPoints(const std::shared_ptr<MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid) {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (int i = 0; i < mesh->nodeCount(); i++) {
        std::array<double, 3> p;
        mesh->nodeCoordinate(i, p.data());
        points->InsertNextPoint(p[0], p[1], p[2]);
    }
    grid->SetPoints(points);
}

void DepsViz::addField(std::shared_ptr<inf::FieldInterface> field) {
    if (field->association() == FieldInterface::NODE) {
        node_field_names.push_back(field->name());
        node_fields.push_back(field);
    } else if (field->association() == FieldInterface::CELL) {
        cell_field_names.push_back(field->name());
        cell_fields.push_back(field);
    } else {
        throw std::logic_error("DepsViz only supports node/cell fields");
    }
}

void DepsViz::addNodeFields(vtkSmartPointer<vtkUnstructuredGrid> grid) {
    unsigned long n = node_fields.size();
    for (int i = 0; i < n; i++) {
        addNodeField(node_fields[i], grid);
    }
}

void DepsViz::addCellFields(vtkSmartPointer<vtkUnstructuredGrid> grid) {
    unsigned long n = cell_fields.size();
    for (int i = 0; i < n; i++) {
        addCellField(cell_fields[i], grid);
    }
}

std::shared_ptr<VizFactory> createVizFactory() { return std::make_shared<HOVizFactory>(); }
