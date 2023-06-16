#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/Snap.h>
#include <t-infinity/InterpolationInterface.h>
#include <t-infinity/FieldTools.h>

class InterpolateCommand : public inf::SubCommand {
  public:
    std::string description() const override {
        return "interpolate solution from mesh A to mesh B";
    }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"interpolate-plugin"},
                       "Interpolator plugin to use <ParfaitInterpolator>",
                       false,
                       "ParfaitInterpolator");
        m.addParameter(inf::Alias::preprocessor(), inf::Help::preprocessor(), false, "default");
        m.addParameter(inf::Alias::plugindir(), inf::Help::plugindir(), false, inf::getPluginDir());
        m.addParameter(inf::Alias::outputFileBase(), "Output file name", false, "out.snap");
        m.addParameter({"source"}, "original mesh", true);
        m.addParameter({"target"}, "target mesh", true);
        m.addParameter({"snap"}, "snap file with solution data", true);
        m.addParameter({"fields"}, "names of fields to interpolate", false);
        m.addFlag({"smooth"}, "Smooth the resulting fields with Taubin");
        m.addFlag({"debug"}, "Add debug fields to output file");
        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto source_mesh = importSourceMesh(m, mp);
        auto target_mesh = importTargetMesh(m, mp);

        std::string plugin_name = m.get("interpolate-plugin");
        mp_rootprint("Performing interpolation search...\n");
        auto interpolator = inf::getInterpolator(
            inf::getPluginDir(), plugin_name, mp.getCommunicator(), source_mesh, target_mesh);
        mp_rootprint("Done!\n");

        auto fields = importFields(mp, source_mesh, m);
        int num_fields = int(fields.size());
        std::map<std::string, std::string> field_name_to_association;
        auto n2c = inf::NodeToCell::build(*source_mesh);
        for (int i = 0; i < num_fields; i++) {
            auto& f = fields[i];
            field_name_to_association[f->name()] = f->association();
            if (f->association() == inf::FieldAttributes::Cell()) {
                fields[i] = inf::FieldTools::cellToNode_volume(*source_mesh, *fields[i], n2c);
            }
        }

        std::vector<std::vector<int>> target_c2c;
        if (m.has("smooth")) {
            target_c2c = inf::CellToCell::build(*target_mesh);
        }

        std::vector<std::shared_ptr<inf::FieldInterface>> interpolated_fields;
        for (auto& f : fields) {
            mp_rootprint("Interpolating field %s ", f->name().c_str());
            auto f_interp = interpolator->interpolate(f);
            mp_rootprint(".. Done.\n");
            if (m.has("smooth")) {
                mp_rootprint("  Smoothing field %s\n", f->name().c_str());
                f_interp =
                    inf::FieldTools::taubinSmoothing(mp, target_mesh, f_interp, target_c2c, {});
            }
            if (field_name_to_association.at(f->name()) == inf::FieldAttributes::Cell() and
                f_interp->association() != inf::FieldAttributes::Cell()) {
                f_interp = inf::FieldTools::nodeToCell(*target_mesh, *f_interp);
            }
            interpolated_fields.push_back(f_interp);
        }
        if (m.has("debug")) {
            interpolator->addDebugFields(interpolated_fields);
        }

        inf::shortcut::visualize(m.get("o"), mp, target_mesh, interpolated_fields);
    }

  private:
    std::vector<Parfait::Point<double>> extractCellCenters(const inf::MeshInterface& mesh) const {
        std::vector<Parfait::Point<double>> query_points(mesh.cellCount(), {0, 0, 0});
        std::vector<int> cell;
        for (int i = 0; i < mesh.cellCount(); i++) {
            mesh.cell(i, cell);
            auto& cell_center = query_points[i];
            double factor = 1.0 / double(cell.size());
            for (int node : cell) {
                Parfait::Point<double> p = mesh.node(node);
                cell_center += p * factor;
            }
        }
        return query_points;
    }

    std::shared_ptr<inf::MeshInterface> importSourceMesh(const Parfait::CommandLineMenu& m,
                                                         MessagePasser mp) const {
        auto source_grid_name = m.get("source");
        mp_rootprint("Importing source grid: %s\n", source_grid_name.c_str());
        return inf::importMesh(source_grid_name, m, mp);
    }

    std::shared_ptr<inf::MeshInterface> importTargetMesh(const Parfait::CommandLineMenu& m,
                                                         MessagePasser mp) const {
        auto target_grid_name = m.get("target");
        mp_rootprint("Loading target grid: %s\n", target_grid_name.c_str());
        return inf::importMesh(target_grid_name, m, mp);
    }
};

CREATE_INF_SUBCOMMAND(InterpolateCommand)
