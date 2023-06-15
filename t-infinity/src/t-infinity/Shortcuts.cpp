#include <vector>
#include <string>
#include <string>
#include "Shortcuts.h"
#include "VisualizePartition.h"
#include <parfait/Throw.h>
#include "Extract.h"
#include "CommonSubcommandFunctions.h"
#include <parfait/VectorTools.h>
#include "LineSampling.h"
#include "FieldTools.h"
#include "parfait/LinePlotters.h"
#include "FieldLoader.h"
#include "MeshShard.h"
#include "MeshExploder.h"

std::vector<std::string> makeFieldNames(std::string base, int num_fields) {
    std::vector<std::string> names(num_fields, base);
    if (num_fields == 1) return names;

    for (int i = 0; i < int(names.size()); i++) {
        names[i] += "_" + std::to_string(i);
    }
    return names;
}

std::shared_ptr<inf::MeshInterface> inf::shortcut::loadMesh(MessagePasser mp,
                                                            std::string filename,
                                                            std::string plugin_name) {
    if (plugin_name == "default") {
        plugin_name = selectDefaultPreProcessorFromMeshFilename(filename);
    }
    return inf::getMeshLoader(getPluginDir(), plugin_name)->load(mp.getCommunicator(), filename);
}
std::vector<std::shared_ptr<inf::FieldInterface>> inf::shortcut::loadFields(
    MessagePasser mp, std::string filename, const MeshInterface& mesh) {
    std::vector<std::shared_ptr<FieldInterface>> fields;
    if (filename.empty()) return fields;
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "solb") {
        auto refine_field_loader = getFieldLoader(inf::getPluginDir(), "RefinePlugins");
        refine_field_loader->load(filename, mp.getCommunicator(), mesh);

        for (auto name : refine_field_loader->availableFields()) {
            mp_rootprint("Extracting field: %s\n", name.c_str());
            fields.emplace_back(refine_field_loader->retrieve(name));
        }
    } else {
        Snap snap(mp.getCommunicator());
        snap.addMeshTopologies(mesh);
        snap.load(filename);
        for (auto& name : snap.availableFields()) {
            mp_rootprint("Extracting field: %s\n", name.c_str());
            fields.emplace_back(snap.retrieve(name));
        }
    }
    return fields;
}
std::shared_ptr<inf::VizInterface> inf::shortcut::loadViz(std::string filename,
                                                          std::shared_ptr<MeshInterface> mesh,
                                                          MessagePasser mp,
                                                          std::string directory,
                                                          std::string plugin_name) {
    if (plugin_name == "default") {
        plugin_name = selectDefaultPostProcessorFromMeshFilename(filename);
    }
    if (directory == "default") {
        directory = getPluginDir();
    }
    auto factory = getVizFactory(directory, plugin_name);
    return factory->create(filename, mesh, mp.getCommunicator());
}
void inf::shortcut::visualize(std::string filename,
                              MessagePasser mp,
                              std::shared_ptr<MeshInterface> mesh,
                              const std::vector<std::shared_ptr<inf::FieldInterface>>& fields,
                              std::string plugin_name) {
    if (plugin_name == "default") {
        plugin_name = selectDefaultPostProcessorFromMeshFilename(filename);
    }
    auto viz = loadViz(filename, mesh, mp, inf::getPluginDir(), plugin_name);
    for (auto f : fields) {
        viz->addField(f);
    }
    viz->visualize();
}
auto inf::shortcut::filterBySurfaceTags(
    MessagePasser mp,
    std::shared_ptr<inf::MeshInterface> mesh,
    const std::vector<int>& tags,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields)
    -> std::tuple<std::shared_ptr<inf::MeshInterface>,
                  std::vector<std::shared_ptr<inf::FieldInterface>>> {
    auto filter = FilterFactory::surfaceTagFilter(mp.getCommunicator(), mesh, tags);
    std::vector<std::shared_ptr<inf::FieldInterface>> output_fields;
    for (auto f : fields) {
        output_fields.push_back(filter->apply(f));
    }
    return {filter->getMesh(), output_fields};
}
void inf::shortcut::visualize_volume(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields) {
    auto filter = FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    for (auto f : fields) viz->addField(filter->apply(f));
    viz->visualize();
}
void inf::shortcut::visualize_surface(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields) {
    auto filter = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    for (auto f : fields) viz->addField(filter->apply(f));
    viz->visualize();
}
void inf::shortcut::visualize_surface_at_cells(std::string filename,
                                               MessagePasser mp,
                                               std::shared_ptr<MeshInterface> mesh,
                                               const std::vector<std::vector<double>>& fields,
                                               const std::vector<std::string>& field_names) {
    auto filter = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    PARFAIT_ASSERT_EQUAL(fields.size(), field_names.size());

    for (int i = 0; i < int(fields.size()); i++) {
        auto f_interface = std::make_shared<inf::VectorFieldAdapter>(
            field_names[i], inf::FieldAttributes::Cell(), 1, fields[i]);
        viz->addField(filter->apply(f_interface));
    }
    viz->visualize();
}
void inf::shortcut::visualize_surface_tags(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    const std::vector<int>& tags,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& field) {
    auto filter = FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    for (auto f : field) {
        viz->addField(filter->apply(f));
    }
    viz->visualize();
}
void inf::shortcut::visualize_surface_tags(std::string filename,
                                           MessagePasser mp,
                                           std::shared_ptr<MeshInterface> mesh,
                                           const std::vector<int>& tags,
                                           const std::vector<std::vector<double>>& cell_fields,
                                           const std::vector<std::string>& field_names) {
    auto filter = FilterFactory::tagFilter(mp.getCommunicator(), mesh, tags);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    for (unsigned long i = 0; i < cell_fields.size(); i++) {
        auto f = std::make_shared<inf::VectorFieldAdapter>(
            field_names[i], inf::FieldAttributes::Cell(), 1, cell_fields[i]);
        viz->addField(filter->apply(f));
    }
    viz->visualize();
}

void inf::shortcut::visualize_with_node_and_cell_ids(std::string filename,
                                                     MessagePasser mp,
                                                     std::shared_ptr<MeshInterface> mesh) {
    auto viz = loadViz(filename, mesh, mp, inf::getPluginDir(), "ParfaitViz");
    {
        auto node_ids = Parfait::VectorTools::to_double(inf::extractGlobalNodeIds(*mesh));
        auto f = std::make_shared<inf::VectorFieldAdapter>(
            "global-node-id", inf::FieldAttributes::Node(), node_ids);
        viz->addField(f);
    }
    {
        auto cell_ids = Parfait::VectorTools::to_double(inf::extractGlobalCellIds(*mesh));
        auto f = std::make_shared<inf::VectorFieldAdapter>(
            "global-cell-id", inf::FieldAttributes::Cell(), cell_ids);
        viz->addField(f);
    }
    viz->visualize();
}
void inf::shortcut::visualize_surface_cells(std::string filename,
                                            MessagePasser mp,
                                            std::shared_ptr<MeshInterface> mesh,
                                            const std::vector<std::vector<double>>& cell_fields,
                                            const std::vector<std::string>& field_names) {
    auto filter = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    auto viz = loadViz(filename, filter->getMesh(), mp, inf::getPluginDir(), "ParfaitViz");
    for (unsigned long i = 0; i < cell_fields.size(); i++) {
        auto f = std::make_shared<inf::VectorFieldAdapter>(
            field_names[i], inf::FieldAttributes::Cell(), 1, cell_fields[i]);
        viz->addField(filter->apply(f));
    }
    viz->visualize();
}
void inf::shortcut::visualize(std::string filename,
                              MessagePasser mp,
                              std::shared_ptr<MeshInterface> mesh,
                              std::string association,
                              const std::vector<std::vector<double>>& fields,
                              const std::vector<std::string>& field_names,
                              std::string plugin_name) {
    auto viz = loadViz(filename, mesh, mp, inf::getPluginDir(), plugin_name);
    for (int i = 0; i < int(fields.size()); i++) {
        auto name = field_names[i];
        viz->addField(std::make_shared<inf::VectorFieldAdapter>(name, association, 1, fields[i]));
    }
    viz->visualize();
}
void inf::shortcut::visualize(std::string filename,
                              MessagePasser mp,
                              std::shared_ptr<MeshInterface> mesh,
                              std::string association,
                              const std::vector<std::vector<double>>& fields,
                              std::string plugin_name) {
    auto names = makeFieldNames("field", fields.size());
    visualize(filename, mp, mesh, association, fields, names, plugin_name);
}
void inf::shortcut::visualize_at_nodes(std::string filename,
                                       MessagePasser mp,
                                       std::shared_ptr<MeshInterface> mesh,
                                       const std::vector<std::vector<double>>& fields,
                                       const std::vector<std::string>& field_names,
                                       std::string plugin_name) {
    auto viz = loadViz(filename, mesh, mp, inf::getPluginDir(), plugin_name);
    for (int i = 0; i < int(fields.size()); i++) {
        auto name = field_names[i];
        viz->addField(std::make_shared<inf::VectorFieldAdapter>(
            name, inf::FieldAttributes::Node(), 1, fields[i]));
    }
    viz->visualize();
}
void inf::shortcut::visualize_volume_at_cells(std::string filename,
                                              MessagePasser mp,
                                              std::shared_ptr<MeshInterface> mesh,
                                              const std::vector<std::vector<double>>& fields,
                                              const std::vector<std::string>& field_names,
                                              std::string plugin_name) {
    std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields;
    for (int i = 0; i < int(fields.size()); i++) {
        auto name = field_names[i];
        inf_fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            name, inf::FieldAttributes::Cell(), 1, fields[i]));
    }
    return visualize_volume(filename, mp, mesh, inf_fields);
}
void inf::shortcut::visualize_at_cells(std::string filename,
                                       MessagePasser mp,
                                       std::shared_ptr<MeshInterface> mesh,
                                       const std::vector<std::vector<double>>& fields,
                                       const std::vector<std::string>& field_names,
                                       std::string plugin_name) {
    auto viz = loadViz(filename, mesh, mp, inf::getPluginDir(), plugin_name);
    for (int i = 0; i < int(fields.size()); i++) {
        auto name = field_names[i];
        viz->addField(std::make_shared<inf::VectorFieldAdapter>(
            name, inf::FieldAttributes::Cell(), 1, fields[i]));
    }
    viz->visualize();
}
void inf::shortcut::visualize_at_nodes(std::string filename,
                                       MessagePasser mp,
                                       std::shared_ptr<MeshInterface> mesh,
                                       const std::vector<std::vector<double>>& fields) {
    visualize_at_nodes(filename, mp, mesh, fields, makeFieldNames("field", fields.size()));
}
void inf::shortcut::visualize_at_cells(std::string filename,
                                       MessagePasser mp,
                                       std::shared_ptr<MeshInterface> mesh,
                                       const std::vector<std::vector<double>>& fields) {
    visualize_at_cells(filename, mp, mesh, fields, makeFieldNames("field", fields.size()));
}
void inf::shortcut::visualize_volume_partitions(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields,
    std::string plugin_name) {
    auto filter = FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
    std::vector<std::shared_ptr<inf::FieldInterface>> filtered_fields;
    for (auto f : inf_fields) filtered_fields.push_back(filter->apply(f));
    visualizePartition(filename, *filter->getMesh(), filtered_fields);
}
void inf::shortcut::visualize_surface_partitions(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields,
    std::string plugin_name) {
    auto filter = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    std::vector<std::shared_ptr<inf::FieldInterface>> filtered_fields;
    for (auto f : inf_fields) filtered_fields.push_back(filter->apply(f));
    visualizePartition(filename, *filter->getMesh(), filtered_fields);
}
void inf::shortcut::visualize_partitions(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields,
    std::string plugin_name) {
    visualizePartition(filename, *mesh, inf_fields);
}

void inf::shortcut::visualize_line_sample(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    Parfait::Point<double> a,
    Parfait::Point<double> b,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields) {
    std::vector<std::vector<double>> fields_vector;
    std::vector<std::string> field_names;
    for (auto f : fields) {
        if (f->association() == inf::FieldAttributes::Node()) {
            fields_vector.push_back(inf::FieldTools::extractColumn(*f, 0));
            field_names.push_back(f->name());
        } else if (f->association() == inf::FieldAttributes::Cell()) {
            auto node_field = inf::FieldTools::cellToNode_volume(*mesh, *f);
            auto nf = inf::FieldTools::extractColumn(*node_field, 0);
            fields_vector.push_back(nf);
            field_names.push_back(f->name());
        } else {
            mp_rootprint("Could not sample field %s\n", f->name().c_str());
        }
    }
    Parfait::DataFrame df = linesampling::sample(mp, *mesh, a, b, fields_vector, field_names);
    auto extension = Parfait::StringTools::getExtension(filename);
    if (mp.Rank() == 0) {
        if (extension == "csv") {
            Parfait::LinePlotters::CSVWriter::write(filename, df);
        } else if (extension == "dat") {
            Parfait::LinePlotters::TecplotWriter::write(filename, df);
        }
    }
}
void inf::shortcut::visualize_exploded(
    std::string filename,
    MessagePasser mp,
    std::shared_ptr<MeshInterface> mesh,
    double scale,
    const std::vector<std::shared_ptr<inf::FieldInterface>>& fields,
    std::string plugin_name) {
    auto exploded_mesh = inf::explodeMesh(mp, mesh, scale);
    visualize(filename, mp, exploded_mesh, fields, plugin_name);
}
std::shared_ptr<inf::MeshInterface> inf::shortcut::shard(MessagePasser mp,
                                                         const inf::MeshInterface& mesh) {
    return getMeshShard(inf::getPluginDir(), "RefinePlugins")->shard(mp.getCommunicator(), mesh);
}
