#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/MetricCalculatorInterface.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/Snap.h>
#include <t-infinity/Gradients.h>
#include <parfait/Dictionary.h>

using namespace inf;

class ScalarToHessianCommand : public SubCommand {
  public:
    std::string description() const override {
        return "convert snap with scalar to snap with hessian";
    }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::snap(), Help::snap(), true);
        m.addParameter({"select"}, "Only include these fields in the output file.", false);
        m.addParameter({"complexity", "c"}, "Target complexity", false, "1000");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "hessian");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        auto fields = importFields(mp, mesh, m);
        auto grad_calculator = NodeToNodeGradientCalculator(mp, mesh);
        auto snap = Snap(mp.getCommunicator());
        snap.addMeshTopologies(*mesh);
        auto metric_calculator = getMetricCalculator(inf::getPluginDir(), "RefinePlugins");
        Parfait::Dictionary metric_settings;
        metric_settings["complexity"] = m.getDouble("complexity");
        for (const auto& f : fields) {
            auto metric =
                metric_calculator->calculate(mesh, mp.getCommunicator(), f, metric_settings.dump());
            snap.add(metric);
        }
        auto output_filename = m.get(Alias::outputFileBase());
        snap.writeFile(output_filename);
    }
};

CREATE_INF_SUBCOMMAND(ScalarToHessianCommand)