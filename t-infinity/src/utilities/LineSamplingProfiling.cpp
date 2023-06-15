// #include <parfait/SetTools.h>
#include <parfait/Timing.h>
// #include <t-infinity/CommonAliases.h>
// #include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/LineSampling.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/MeshExtents.h>
#include <Tracer.h>

namespace inf  {
class LineSamplingProfilingCommand : public SubCommand {
  public:
    std::string description() const override { return "Profile the performance of line sampling"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"--mesh", "-m"}, "mesh used for the test", true);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {

        auto mesh = inf::shortcut::loadMesh(mp, m.get("--mesh"));

        auto face_neighbors = inf::FaceNeighbors::build(*mesh);
        auto overall_domain = inf::meshExtent(mp, *mesh);
        auto partition_domain = inf::partitionExtent(*mesh);
        Parfait::Adt3DExtent adt(partition_domain);
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto cell_extent = Parfait::ExtentBuilder::buildExtentForCellInMesh(*mesh, c);
            adt.store(c, cell_extent);
        }

        // set the a and be to the farthest apart domain extent corners for worst case
        Parfait::Point<double> a, b;
        auto longest_distance = std::numeric_limits<double>::min();
        for (const auto& c1 : overall_domain.corners()) {
            for (const auto& c2 : overall_domain.corners()) {
                auto dist = c1.distance(c2);
                if (dist > longest_distance) {
                    longest_distance = dist;
                    a = c1;
                    b = c2;
                }
            }
        }
        std::vector<double> test_field(mesh->nodeCount());
        auto fieldCallback = [](double x, double y, double z) { return 3.0 * x; };
        for (int n = 0; n < mesh->nodeCount(); n++) {
            auto p = mesh->node(n);
            test_field[n] = fieldCallback(p[0], p[1], p[2]);
        }

        // normal cut
        auto start_time_standard = Parfait::Now();
        Tracer::begin("Standard Line Sample");
        auto df_standard = inf::linesampling::sample(mp, *mesh, face_neighbors, a, b, {test_field}, {"test"});
        Tracer::end("Standard Line Sample");
        auto end_time_standard = Parfait::Now();
        Tracer::begin("ADT Line Sample");
        auto df_adt = inf::linesampling::sample(mp, *mesh, face_neighbors, adt, a, b, {test_field}, {"test"});
        Tracer::end("ADT Line Sample");
        auto end_time_adt = Parfait::Now();
        std::cout << "Standard sample took " << Parfait::readableElapsedTimeAsString(start_time_standard, end_time_standard) << " s\n";
        std::cout << "Adt sample took " << Parfait::readableElapsedTimeAsString(end_time_standard, end_time_adt) << " s\n";

        PARFAIT_ASSERT(df_standard["x"].size() == df_adt["x"].size(), "Samples sizes not equal: " + std::to_string(df_standard["x"].size()) + " vs " + std::to_string(df_adt["x"].size()));
    }

};
}

CREATE_INF_SUBCOMMAND(inf::LineSamplingProfilingCommand)