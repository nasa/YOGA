#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <YogaConfiguration.h>
#include <parfait/FileTools.h>
#include <t-infinity/Snap.h>
#include <t-infinity/MetricHelpers.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MetricManipulator.h>

using namespace YOGA;
using namespace inf;
using namespace Parfait;

class RotateMetricCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Check syntax for input files"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::mesh(), Help::mesh(),true);
        m.addParameter({"metric"},"metric snap-file",true);
        m.addParameter({"axis"},"x1 y1 z1 x1 y2 z2", true);
        m.addParameter({"degrees"},"degrees to rotate",true);
        m.addParameter(Alias::outputFileBase(),Help::outputFileBase(),false,"rotated-metric.snap");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto axis_entries = m.getAsVector("axis");
        if(axis_entries.size() != 6) return;
        Parfait::Point<double> a,b;
        a[0] = Parfait::StringTools::toDouble(axis_entries[0]);
        a[1] = Parfait::StringTools::toDouble(axis_entries[1]);
        a[2] = Parfait::StringTools::toDouble(axis_entries[2]);
        b[0] = Parfait::StringTools::toDouble(axis_entries[3]);
        b[1] = Parfait::StringTools::toDouble(axis_entries[4]);
        b[2] = Parfait::StringTools::toDouble(axis_entries[5]);
        double degrees = m.getDouble("degrees");
        MotionMatrix rotation;
        rotation.addRotation(a,b,degrees);

        auto meshfile = m.get(Alias::mesh());
        auto mesh_loader = selectDefaultPreProcessorFromMeshFilename(meshfile);
        auto mesh = shortcut::loadMesh(mp, meshfile, mesh_loader);
        auto metric_field = loadMetricFromSnapFile(mp, *mesh, m.get("metric"));

        auto metrics = MetricManipulator::fromFieldToTensors(*metric_field);
        for(auto& tensor:metrics)
            tensor = MetricTensor::rotate(tensor,rotation);

        metric_field = MetricManipulator::toNodeField(metrics);

        Snap snap(mp.getCommunicator());
        snap.addMeshTopologies(*mesh);
        snap.add(metric_field);
        auto output_filename = m.get("o");
        snap.writeFile(output_filename);
    }

  private:

};

CREATE_INF_SUBCOMMAND(RotateMetricCommand)
