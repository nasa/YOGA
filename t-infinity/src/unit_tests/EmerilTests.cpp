#include <RingAssertions.h>
#include <t-infinity/RecipeGenerator.h>
#include "MockMeshes.h"
#include <dlfcn.h>
#include <t-infinity/VizInterface.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/FieldExtractor.h>
#include <t-infinity/Emeril.h>

using namespace inf;

class MockFieldExtractor : public FieldExtractor {
  public:
    MockFieldExtractor(int node_count, int cell_count) : node_count(node_count), cell_count(cell_count) {}
    std::shared_ptr<FieldInterface> extract(const std::string& name) override {
        if ("bacon" == name) {
            std::vector<double> v(node_count);
            return std::make_shared<VectorFieldAdapter>(name, FieldAttributes::Node(), 1, v);
        } else {
            std::vector<double> v(cell_count);
            return std::make_shared<VectorFieldAdapter>(name, FieldAttributes::Node(), 1, v);
        }
    }

  private:
    int node_count;
    int cell_count;
};

class VizSpy : public VizInterface {
  public:
    VizSpy(std::string filename, std::shared_ptr<MeshInterface> m) : base(filename), mesh(m) {}
    void visualize() { vis_called = true; }
    void addField(std::shared_ptr<FieldInterface> field) {
        if (field->association() == FieldAttributes::Node()) {
            REQUIRE(mesh->nodeCount() == field->size());
        } else {
            REQUIRE(mesh->cellCount() == field->size());
        }
        field_names.push_back(field->name());
    }
    bool wasVisualizeCalled() { return vis_called; }
    int filteredMeshCellCount() { return mesh->cellCount(); }

    std::string filenameBase() { return base; }
    std::vector<std::string> fieldNames() { return field_names; }

  private:
    bool vis_called = false;
    std::string base = "";
    std::vector<std::string> field_names;
    std::shared_ptr<MeshInterface> mesh;
};

class EmerilTestShunt : public Emeril {
  public:
    EmerilTestShunt(std::shared_ptr<MeshInterface> m, std::shared_ptr<FieldExtractor> field_extractor, MessagePasser mp)
        : Emeril(m, field_extractor, mp) {}

    bool didCallVisualize() { return spy->wasVisualizeCalled(); }
    int filteredMeshCellCount() { return spy->filteredMeshCellCount(); }
    std::string filenameBase() { return spy->filenameBase(); }
    std::vector<std::string> fieldNames() { return spy->fieldNames(); }

  private:
    std::shared_ptr<VizSpy> spy = std::make_shared<VizSpy>("", nullptr);
    std::shared_ptr<VizInterface> createVizObject(const std::string& plugin,
                                                  const std::string& plugin_directory,
                                                  const std::string& output_name,
                                                  std::shared_ptr<MeshInterface> m,
                                                  MessagePasser mp) override {
        spy = std::make_shared<VizSpy>(output_name, m);
        return spy;
    }
};

TEST_CASE("Given no recipes, make nothing") {
    auto mesh = mock::getSingleTetMesh();
    std::vector<RecipeGenerator::Recipe> no_recipes;

    MessagePasser mp(MPI_COMM_WORLD);
    auto field_extractor = std::make_shared<MockFieldExtractor>(mesh->nodeCount(), mesh->cellCount());
    EmerilTestShunt test_shunt(mesh, field_extractor, mp);
    test_shunt.make(no_recipes);
    auto names = test_shunt.fieldNames();
    REQUIRE(names.size() == 0);
    REQUIRE_FALSE(test_shunt.didCallVisualize());
}

TEST_CASE("Given a recipe, make it") {
    auto mesh = mock::getSingleTetMesh();
    std::string order = "select fields bacon eggs ";
    order += "select surfaces ";
    order += "save as bam";
    REQUIRE(order.size() > 0);
    auto recipes = inf::RecipeGenerator::parse(order);

    MessagePasser mp(MPI_COMM_WORLD);
    auto field_extractor = std::make_shared<MockFieldExtractor>(mesh->nodeCount(), mesh->cellCount());
    EmerilTestShunt test_shunt(mesh, field_extractor, mp);
    test_shunt.make(recipes);
    REQUIRE(test_shunt.filenameBase() == "bam");
    auto names = test_shunt.fieldNames();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "bacon");
    REQUIRE(names[1] == "eggs");
    REQUIRE(test_shunt.didCallVisualize());

    REQUIRE(4 == test_shunt.filteredMeshCellCount());
}
