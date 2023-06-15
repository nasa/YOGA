#pragma once

#include <t-infinity/FieldInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VizInterface.h>

class vtkUnstructuredGrid;
class vtkCell;
template <class T>
class vtkSmartPointer;

class DepsViz : public inf::VizInterface {
  public:
    DepsViz(std::string filename, const std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm);
    virtual void addField(std::shared_ptr<inf::FieldInterface> field) override;
    virtual void visualize() override;

  private:
    template <typename T>
    void addNodeField_byType(std::shared_ptr<inf::FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid);

    template <typename T>
    void addCellField_byType(std::shared_ptr<inf::FieldInterface> f, vtkSmartPointer<vtkUnstructuredGrid> grid);

    void copyGridToVtk(const std::shared_ptr<inf::MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid);

    std::vector<int> mapToVtkCellTypes(const std::shared_ptr<inf::MeshInterface>& mesh);

    vtkSmartPointer<vtkCell> extractVtkCell(const std::shared_ptr<inf::MeshInterface>& mesh,
                                            inf::MeshInterface::CellType type,
                                            int cell_id);
    void setCells(const std::shared_ptr<inf::MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid);
    void setPoints(const std::shared_ptr<inf::MeshInterface>& mesh, vtkSmartPointer<vtkUnstructuredGrid> grid);
    void addNodeFields(vtkSmartPointer<vtkUnstructuredGrid> grid);
    void addCellFields(vtkSmartPointer<vtkUnstructuredGrid> grid);
    void addNodeField(std::shared_ptr<inf::FieldInterface> field, vtkSmartPointer<vtkUnstructuredGrid> grid);
    void addCellField(std::shared_ptr<inf::FieldInterface> field, vtkSmartPointer<vtkUnstructuredGrid> grid);

  private:
    std::string filename_base;
    const std::shared_ptr<inf::MeshInterface> mesh;
    MessagePasser mp;

    std::vector<std::string> node_field_names;
    std::vector<std::shared_ptr<inf::FieldInterface>> node_fields;

    std::vector<std::string> cell_field_names;
    std::vector<std::shared_ptr<inf::FieldInterface>> cell_fields;

    void addCellTags(vtkSmartPointer<vtkUnstructuredGrid> pointer);
};

class HOVizFactory : public inf::VizFactory {
  public:
    virtual std::shared_ptr<inf::VizInterface> createViz(std::string name,
                                                         std::shared_ptr<inf::MeshInterface> mesh,
                                                         MPI_Comm comm) const override;
};

extern "C" {
std::shared_ptr<inf::VizFactory> createVizFactory();
};
