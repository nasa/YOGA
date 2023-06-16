#pragma once

#include <t-infinity/FieldInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VizInterface.h>
#include <parfait/VTKWriter.h>

class ParfaitViz : public inf::VizInterface {
  public:
    ParfaitViz(std::string filename, const std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm);
    virtual void addField(std::shared_ptr<inf::FieldInterface> field) override;
    virtual void visualize() override;

  private:
    static constexpr int root = 0;
    std::string filename;
    std::shared_ptr<inf::MeshInterface> mesh;
    MessagePasser mp;
    std::vector<long> owned_global_cell_ids;
    std::vector<long> owned_global_node_ids;
    std::vector<std::shared_ptr<inf::FieldInterface>> node_field_ptrs;
    std::vector<std::shared_ptr<inf::FieldInterface>> cell_field_ptrs;
    void addNodeField(const std::shared_ptr<inf::FieldInterface>& field);
    void addCellField(const std::shared_ptr<inf::FieldInterface>& field);

    void writeUsingSerialWriter();
    void writeSTL();
    void writeSU2();
    void writePoints3D();
    void writeUsingParallelTecplotWriter();
    void writeUsingParallelVTKWriter();
    void writeUsingSzPlt();
    void writeUsingParallelUgridWriter();
    void writeSnapDatabase();
    void writeUsingDataFrameWriter();
    void renameFileIfAsciiTecplotRequested();
    void writeTetGen();
    bool areThereFieldsOtherThanCellTag();
};

class ParfaitVizFactory : public inf::VizFactory {
  public:
    virtual std::shared_ptr<inf::VizInterface> create(std::string name,
                                                      std::shared_ptr<inf::MeshInterface> mesh,
                                                      MPI_Comm comm) const override;
};

extern "C" {
std::shared_ptr<inf::VizFactory> createVizFactory();
}
