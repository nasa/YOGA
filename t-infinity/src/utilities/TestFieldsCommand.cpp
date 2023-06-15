#include <t-infinity/SubCommand.h>
#include <map>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/Extract.h>
#include <t-infinity/FilterFactory.h>
#include <parfait/OmlParser.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Snap.h>
#include <t-infinity/ScriptVisualization.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <parfait/SCurve.h>

using namespace inf;

std::function<double(double, double, double)> getShockX(double shock_x_location = 0.5) {
    double very_steep = 1000.0;
    auto s_curve = Parfait::SCurve::create(0.1, 1.0, shock_x_location, very_steep);

    auto f = [=](double x, double y, double z) { return s_curve(x); };
    return f;
}

std::function<double(double, double, double)> getSinesAndCosines() {
    double pi = 4.0 * atan(1.0);
    auto f = [=](double x, double y, double z) { return sin(pi * x) + cos(pi * y) - sin(pi * z); };
    return f;
}

std::shared_ptr<inf::VectorFieldAdapter> createScalarTestFieldAtNodes(
    const inf::MeshInterface& mesh, std::function<double(double, double, double)> function) {
    std::vector<double> scalar_field(mesh.nodeCount());
    for (int i = 0; i < mesh.nodeCount(); i++) {
        auto p = mesh.node(i);
        scalar_field[i] = function(p[0], p[1], p[2]);
    }
    return std::make_shared<inf::VectorFieldAdapter>(
        "scalar", inf::FieldAttributes::Node(), scalar_field);
}

std::shared_ptr<inf::VectorFieldAdapter> createScalarTestFieldAtCells(
    const inf::MeshInterface& mesh, std::function<double(double, double, double)> function) {
    std::vector<double> scalar_field(mesh.cellCount());
    for (int i = 0; i < mesh.cellCount(); i++) {
        inf::Cell cell(mesh, i);
        auto p = cell.averageCenter();
        scalar_field[i] = function(p[0], p[1], p[2]);
    }
    return std::make_shared<inf::VectorFieldAdapter>(
        "scalar", inf::FieldAttributes::Cell(), scalar_field);
}

class TestFieldsCommand : public SubCommand {
  public:
    std::string description() const override { return "Generate test fields on a mesh"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::mesh(), Help::mesh(), false);
        m.addParameter({"at"}, "Generate field at [nodes, cells]", false, "nodes");
        m.addParameter({"-o"}, "Output snap filename", false, "out.snap");
        m.addParameter({"--field"}, "The output field you want [sines, x-shock]", false, "sines");
        m.addParameter(
            {"--multi-element"}, "Create non-scalar fields of desired length", false, "1");
        return m;
    }

    std::function<double(double, double, double)> getTestFunction(std::string function_name) {
        if (function_name == "sin" or function_name == "sine") {
            return getSinesAndCosines();
        }
        if (function_name == "x-shock" or function_name == "shock") {
            return getShockX();
        }
        return getSinesAndCosines();
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = inf::importMesh(m, mp);
        bool at_nodes = inf::atNodes(m);
        bool at_cells = inf::atCells(m);
        PARFAIT_ASSERT(at_nodes or at_cells, "Must make fields at nodes or cells");
        inf::Snap snap(mp);
        snap.addMeshTopologies(*mesh);

        int num_fields = m.getInt("--multi-element");

        auto test_function = getTestFunction(m.get("--field"));
        std::vector<std::shared_ptr<inf::FieldInterface>> field_components;
        for (int i = 0; i < num_fields; i++) {
            std::shared_ptr<inf::FieldInterface> f = nullptr;
            if (at_cells) {
                f = createScalarTestFieldAtCells(*mesh, test_function);
            } else {
                f = createScalarTestFieldAtNodes(*mesh, test_function);
            }
            field_components.push_back(f);
        }

        auto merged_field = inf::FieldTools::merge(field_components, "test");
        snap.add(merged_field);
        snap.writeFile(m.get("o"));
    }
};

CREATE_INF_SUBCOMMAND(TestFieldsCommand)