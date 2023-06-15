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

using namespace inf;

class SnapCommand : public SubCommand {
  public:
    std::string description() const override { return "Manipulate snap files"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter({"snap", "s"}, {"List of snap files to load"}, false, "");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::mesh(), Help::mesh(), false);
        m.addParameter({"verbose", "v"}, "Print verbose summary", false);
        m.addParameter({"at"}, "Shows the association of each field", false);
        m.addFlag({"example"}, "Writes an example rename file");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false);
        m.addParameter({"rename"}, "rename <old_name> <new_name>", false);
        m.addParameter({"rename-from-file"},
                       "renames multiple fields at once using an ascii file for the name mapping",
                       false);
        m.addParameter({"delete"}, "delete a list of fields from the output file", false);
        return m;
    }

    std::vector<std::shared_ptr<VectorFieldAdapter>> loadFields(Parfait::CommandLineMenu m,
                                                                MessagePasser mp) {
        std::vector<std::string> filenames = m.getAsVector(Alias::snap());
        mp_rootprint("Loading snap files: <%s>\n", Parfait::to_string(filenames).c_str());
        std::vector<std::shared_ptr<VectorFieldAdapter>> original_fields;
        for (auto filename : filenames) {
            inf::Snap snap(mp.getCommunicator());
            snap.setDefaultTopology(filename);
            snap.load(filename);
            for (auto name : snap.availableFields()) {
                auto f = snap.retrieve(name);
                std::shared_ptr<VectorFieldAdapter> g =
                    std::dynamic_pointer_cast<VectorFieldAdapter>(f);
                original_fields.push_back(g);
            }
        }
        return original_fields;
    }

    void writeExampleRename(Parfait::CommandLineMenu m, MessagePasser mp) {
        if (mp.Rank() == 0) {
            auto fields = loadFields(m, mp);
            std::map<std::string, std::string> dict;
            std::string example_file;
            example_file += "# Example rename file\n";
            example_file += "# Hashes are comments\n";
            example_file += "# field names are in quotes\n";
            example_file += "# old name goes on the left, new name on the right:\n";
            example_file += "#\"my_old_field\" = \"my_new_field\"\n";
            example_file += "# \n";
            example_file += "# You can delete a field by naming it DELETE\n";
            example_file += "#\"Density\" = \"DELETE\"\n";

            example_file += "# Below is a starting point for your snap file(s)\n";
            for (auto f : fields) {
                example_file += "\"" + f->name() + "\"=\"" + f->name() + "_new\"\n";
            }
            example_file += "\n";
            mp_rootprint("Writing an example rename file: example.oml\n");
            Parfait::FileTools::writeStringToFile("example.oml", example_file);
        }
    }

    void handleFileOutput(Parfait::CommandLineMenu m, MessagePasser mp) {
        auto original_fields = loadFields(m, mp);
        std::map<std::string, std::string> old_name_to_new_name;
        for (auto& f : original_fields) {
            auto name = f->name();
            old_name_to_new_name[name] = name;  // by default, nothing changes.
        }

        if (m.has("rename")) {
            auto rename_vector = m.getAsVector("rename");
            PARFAIT_ASSERT(rename_vector.size() == 2,
                           "rename requires 2 arguments, but was passed " +
                               std::to_string(rename_vector.size()));
            auto old_name = rename_vector[0];
            auto new_name = rename_vector[1];
            old_name_to_new_name[old_name] = new_name;
        }

        if (m.has("rename-from-file")) {
            auto rename_filename = m.get("rename-from-file");
            auto dictionary =
                Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString(rename_filename));
            for (auto old_name : dictionary.keys()) {
                auto new_name = dictionary.at(old_name).asString();
                old_name_to_new_name[old_name] = new_name;
            }
        }

        if (m.has("delete")) {
            auto delete_vector = m.getAsVector("delete");
            for (auto name : delete_vector) {
                old_name_to_new_name[name] = "DELETE";
            }
        }

        inf::Snap snap(mp.getCommunicator());
        snap.setDefaultTopology(m.get(Alias::snap()));
        for (auto f : original_fields) {
            auto new_name = old_name_to_new_name.at(f->name());
            mp_rootprint("%s -> %s\n", f->name().c_str(), new_name.c_str());
            if (new_name != "DELETE") {
                f->setAdapterAttribute(FieldAttributes::name(), new_name);
                snap.add(f);
            }
        }

        auto output_filename = m.get(Alias::outputFileBase());
        mp_rootprint("Writing: %s\n", output_filename.c_str());
        snap.writeFile(output_filename);
    }
    void handleConsoleOutput(Parfait::CommandLineMenu m, MessagePasser mp) {
        if (mp.Rank() == 0) {
            auto filename = m.get(Alias::snap());
            inf::Snap snap(mp.getCommunicator());
            auto summary = snap.readSummary(filename);
            printf("Snap File  : %s\n", filename.c_str());
            printf("Field count: %zu\n", summary.size());
            for (size_t i = 0; i < summary.size(); i++) {
                auto attributes = summary.at(i);
                if (m.has("verbose")) {
                    printf("\n");
                    for (auto& pair : attributes) {
                        printf("%s:\n   %s\n", pair.first.c_str(), pair.second.c_str());
                    }
                } else {
                    if (m.has("at")) {
                        printf("\"%s\" at %ss\n",
                               attributes.at("name").c_str(),
                               attributes.at(inf::FieldAttributes::Association()).c_str());
                    } else {
                        printf("\"%s\"\n", attributes.at("name").c_str());
                    }
                }
            }
        }
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        bool writing_file = m.has(Alias::outputFileBase());
        bool writing_example_rename = m.has("example");

        if (writing_file) {
            handleFileOutput(m, mp);
        } else if (writing_example_rename) {
            writeExampleRename(m, mp);
        } else {
            handleConsoleOutput(m, mp);
        }
    }
};

CREATE_INF_SUBCOMMAND(SnapCommand)