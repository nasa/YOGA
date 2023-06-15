#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/Shortcuts.h>
#include <parfait/DataFrame.h>
#include <parfait/LinePlotters.h>
#include <parfait/PointWriter.h>
#include <t-infinity/Snap.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <parfait/SetTools.h>

#include <parfait/TecplotBinaryReader.h>

using namespace inf;

class TecplotToSnapCommand : public SubCommand {
  public:
    std::string description() const override {
        return "Read a tecplot and turn it into a mesh and snap";
    }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"--input", "-i"}, "Input tecplot file", true);
        m.addParameter({"--output", "-o"}, "output file base", true);
        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto filename = m.get("input");
        auto df = Parfait::readFile(filename);
        auto columns = df.columns();
        std::vector<std::vector<double>> fields;
        std::vector<std::string> field_names;
        std::vector<double> x, y, z;
        for (auto c : columns) {
            auto cl = Parfait::StringTools::tolower(c);
            if (cl == "x") {
                x = df[c];
            } else if (cl == "y") {
                y = df[c];
            } else if (cl == "z") {
                z = df[c];
            } else {
                fields.push_back(df[c]);
                field_names.push_back(c);
            }
        }

        // write the point cloud data
        int num_points = x.size();
        std::vector<Parfait::Point<double>> points(num_points);
        for (int i = 0; i < num_points; i++) {
            points[i][0] = x[i];
            points[i][1] = y[i];
            points[i][2] = z[i];
        }
        auto output_basename = Parfait::StringTools::stripExtension(m.get("--output"), std::vector<std::string>({"3D", "dat", "prf"}));
        Parfait::PointWriter::write(output_basename + ".3D", points);

        // set up fields for snap write
        std::vector<std::shared_ptr<inf::VectorFieldAdapter>> fields_base;
        int index = 0;
        for (auto f : fields) {
            fields_base.push_back(std::make_shared<inf::VectorFieldAdapter>(field_names.at(index++), inf::FieldAttributes::Node(), f));
        }

        // set up snap topology
        inf::TinfMeshData mesh_data;
        mesh_data.points = points;
        auto global_node_id_set = Parfait::SetTools::range<long>(0,points.size());
        mesh_data.global_node_id = std::vector<long>(global_node_id_set.begin(), global_node_id_set.end());
        mesh_data.node_owner = std::vector<int>(points.size(), mp.Rank());
        std::shared_ptr<inf::TinfMesh> point_cloud_mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());
        auto snap = inf::Snap(mp);
        snap.addMeshTopologies(*point_cloud_mesh);
        for (auto& f : fields_base) snap.add(f);
        snap.writeFile(output_basename + ".snap");
    }
};

CREATE_INF_SUBCOMMAND(TecplotToSnapCommand)
