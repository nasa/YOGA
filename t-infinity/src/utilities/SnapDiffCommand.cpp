#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/Extract.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Snap.h>
#include <t-infinity/ScriptVisualization.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <parfait/MapTools.h>

using namespace inf;

class SnapDiffCommand : public SubCommand {
  public:
    std::string description() const override { return "Compare two snap files"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter({"--snap-1"}, "The 1st snap file", true);
        m.addParameter({"--snap-2"}, "The 2nd snap file", true);
        m.addParameter({"--tolerance"}, "The difference tolerance", false, "1e-10");
        m.addParameter({"o"}, "Output filename", false, "diff");
        m.addFlag({"volume", "v"}, "Filter to volume cells");
        return m;
    }

    struct FieldLocator {
        std::string filename;
        std::string fieldname;
    };

    FieldLocator parseFieldLocator(const std::vector<std::string>& args) {
        FieldLocator locator;
        bool set_file = false;
        bool set_field = false;
        for (unsigned long i = 0; i < args.size(); i++) {
            auto& a = args[i];
            if (a == "from" or a == "file") {
                locator.filename = args[++i];
                set_file = true;
            }
            if (a == "select" or a == "field") {
                locator.fieldname = args[++i];
                set_field = true;
            }
        }

        if (set_field and set_file)
            return locator;
        else
            PARFAIT_THROW("Could not parse field location " + Parfait::to_string(args));
    }

    std::shared_ptr<FieldInterface> importField(MessagePasser mp,
                                                std::shared_ptr<MeshInterface> mesh,
                                                Parfait::CommandLineMenu m,
                                                std::string filename,
                                                std::string fieldname) {
        auto snap_alias = Alias::snap();
        std::vector<std::shared_ptr<FieldInterface>> fields;
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension == "solb") {
            appendFieldsFromSolb(mp, filename, *mesh, m, fields);
        } else {
            appendFieldsFromSnap(mp, filename, *mesh, m, fields);
        }
        for (auto f : fields)
            if (f->name() == fieldname) return f;
        PARFAIT_THROW("Could not find field " + fieldname + " from file " + fieldname);
    }
    std::map<std::string, std::shared_ptr<inf::FieldInterface>> load(MessagePasser mp,
                                                                     std::string snap_filename,
                                                                     const MeshInterface& mesh) {
        Snap snap(mp.getCommunicator());
        snap.addMeshTopologies(mesh);
        snap.load(snap_filename);

        std::map<std::string, std::shared_ptr<inf::FieldInterface>> fields;
        for (auto f : snap.availableFields()) {
            fields[f] = snap.retrieve(f);
        }
        return fields;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        auto filename_1 = m.get("--snap-1");
        auto filename_2 = m.get("--snap-2");

        auto snap_1 = load(mp, filename_1, *mesh);
        auto snap_2 = load(mp, filename_2, *mesh);

        if (m.has("volume")) {
            auto filter = inf::FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
            mesh = filter->getMesh();
            for (auto f : Parfait::MapTools::keys(snap_1)) {
                snap_1[f] = filter->apply(snap_1.at(f));
            }
            for (auto f : Parfait::MapTools::keys(snap_2)) {
                snap_2[f] = filter->apply(snap_2.at(f));
            }
        }

        auto do_own = inf::extractDoOwnCells(*mesh);
        std::vector<std::shared_ptr<FieldInterface>> diffs;
        for (auto name : Parfait::MapTools::keys(snap_1)) {
            auto f1 = snap_1.at(name);
            if (snap_2.count(name)) {
                auto f2 = snap_2.at(name);
                diffs.push_back(inf::FieldTools::diff(*f1, *f2));
                auto norm = inf::FieldTools::norm(mp, do_own, *diffs.back(), 2.0);
                mp_rootprint("Field %s L2 diff %e\n", name.c_str(), norm);
            } else {
                mp_rootprint("Field %s in %s not present in %s -- skipping\n",
                             name.c_str(),
                             filename_1.c_str(),
                             filename_2.c_str());
            }
        }

        if (m.has("o")) inf::exportMeshAndFields(m.get("o"), mesh, m, mp, diffs);
    }
};

CREATE_INF_SUBCOMMAND(SnapDiffCommand)