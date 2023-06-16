#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Extract.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/Snap.h>
#include <t-infinity/FieldTools.h>
#include <parfait/FileTools.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/IsoSampling.h>
#include <t-infinity/VizFromDictionary.h>
#include <parfait/OmlParser.h>

using namespace inf;

class SamplingCommand : public SubCommand {
  public:
    std::string description() const override { return "slice/filter meshes/field data & export to files"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "out");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::tags(), "Sample only on listed surface tags", false);
        m.addParameter(Alias::select(), "Only load selected fields", false);
        m.addParameter({"at"}, "Convert fields to either <nodes> or <cells>", false);
        m.addParameter({"--script", "-s"}, "Create visualizations from script", false);
        m.addParameter({"--sphere"}, "x y z r = clip to sphere centered at x,y,z with radius r", false);
        m.addParameter(
            {"--plane"}, "x1 y1 z1 x2 y2 z2 = slice on plane centered at x1,y1,z1 with normal x2 y2 z2", false);
        m.addParameter({"--iso-surface"}, "<name> create an isosurface of a field", false);
        m.addParameter({"--iso-value"}, "The value to create isosurface of field from --iso-surface", false, "1.0");
        m.addParameter(Alias::snap(), Help::snap(), false);
        return m;
    }

    bool atNodes(Parfait::CommandLineMenu m) const {
        if (m.has("at")) {
            auto where = m.get("at");
            where = Parfait::StringTools::tolower(where);
            if (where == "node" or where == "nodes") return true;
        }
        return false;
    }

    bool atCells(Parfait::CommandLineMenu m) const {
        if (m.has("at")) {
            auto where = m.get("at");
            where = Parfait::StringTools::tolower(where);
            if (where == "cell" or where == "cells") return true;
        }
        return false;
    }
    void convertToNodes(MessagePasser mp,
                        const MeshInterface& mesh,
                        std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        if (inf::is2D(mp, mesh)) {
            for (auto& field : fields)
                if (field->association() == FieldAttributes::Cell())
                    field = FieldTools::cellToNode_simpleAverage(mesh, *field);
        } else {
            for (auto& field : fields)
                if (field->association() == FieldAttributes::Cell())
                    field = FieldTools::cellToNode_volume(mesh, *field);
        }
    }

    void convertToCells(const MeshInterface& mesh, std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        for (auto& field : fields)
            if (field->association() == FieldAttributes::Node()) field = FieldTools::nodeToCell(mesh, *field);
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        std::vector<std::shared_ptr<FieldInterface>> fields;
        if (m.has("snap")) {
            auto snap_filename = m.get("snap");
            fields = importFields(mp, mesh, m, snap_filename);
        }

        if (m.has("tags")) {
            auto tags = getTags(m);
            std::tie(mesh, fields) = inf::shortcut::filterBySurfaceTags(mp, mesh, tags, fields);
        }
        if (atNodes(m)) convertToNodes(mp, *mesh, fields);
        if (atCells(m)) convertToCells(*mesh, fields);

        if (m.has("script")) {
            auto script_name = m.get("script");
            auto dict = Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString(script_name));
            inf::visualizeFromDictionary(mp, mesh, fields, dict);
            return;
        }

        if (m.has("--sphere")) {
            auto doubles_string = m.getAsVector("--sphere");
            std::vector<double> doubles;
            for (auto s : doubles_string) {
                doubles.push_back(Parfait::StringTools::toDouble(s));
            }
            Parfait::Dictionary dict;
            dict["type"] = "sphere";
            dict["center"] = std::vector<double>{doubles[0], doubles[1], doubles[2]};
            dict["radius"] = doubles[3];
            dict["filename"] = m.getAsVector("o");
            mp_rootprint("Visualizing sphere\n");
            visualizeFromDictionary(mp, mesh, fields, dict);

        } else if (m.has("--plane")) {
            auto doubles_string = m.getAsVector("--plane");
            std::vector<double> doubles;
            for (auto s : doubles_string) {
                doubles.push_back(Parfait::StringTools::toDouble(s));
            }
            Parfait::Dictionary dict;
            dict = Parfait::Dictionary();
            dict["type"] = "plane";
            dict["center"] = std::vector<double>{doubles[0], doubles[1], doubles[2]};
            dict["normal"] = std::vector<double>{doubles[3], doubles[4], doubles[5]};
            dict["filename"] = m.getAsVector("o");
            mp_rootprint("Visualizing plane\n");
            visualizeFromDictionary(mp, mesh, fields, dict);
            return;
        } else if (m.has("iso-surface")) {
            auto field_name = m.get("iso-surface");
            double value = m.getDouble("iso-value");
            std::shared_ptr<inf::FieldInterface> iso_field = nullptr;
            printf("Creating isosurface of field %s at %lf\n", field_name.c_str(), value);
            for (auto& f : fields) {
                if (f->name() == field_name) {
                    iso_field = f;
                    break;
                }
            }
            PARFAIT_ASSERT(iso_field != nullptr, "Could not find requested field");
            auto get_iso_surface_value = isosampling::field_isosurface(*iso_field, value);
            auto iso = inf::isosampling::Isosurface(mp, mesh, get_iso_surface_value);
            auto filtered_mesh = iso.getMesh();
            std::vector<std::shared_ptr<FieldInterface>> iso_fields;
            for (auto& f : fields) iso_fields.push_back(iso.apply(f));

            for (auto filename : m.getAsVector("o")) {
                inf::shortcut::visualize(m.get("o"), mp, filtered_mesh, iso_fields);
            }
            return;
        } else {
            inf::shortcut::visualize(m.get("o"), mp, mesh, fields);
        }
    }
};

CREATE_INF_SUBCOMMAND(SamplingCommand)