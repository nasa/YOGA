
#include "CommonSubcommandFunctions.h"
#include "CommonAliases.h"
#include "TinfMesh.h"
#include "FieldLoader.h"
#include "MeshLoader.h"
#include "VizFactory.h"
#include <t-infinity/PluginLocator.h>
#include <Tracer.h>

namespace inf {
std::vector<std::string> getOutputFieldNames(const Parfait::CommandLineMenu& m,
                                             const std::vector<std::string>& available_fields) {
    if (m.has("select")) {
        return m.getAsVector("select");
    }
    return available_fields;
}
std::shared_ptr<MeshInterface> importMesh(MessagePasser mp,
                                          std::string mesh_filename,
                                          std::string mesh_loader_plugin_name,
                                          bool verbose,
                                          std::string plugin_dir) {
    TRACER_PROFILE_FUNCTION
    auto dir = inf::getPluginDir();
    if (not plugin_dir.empty()) dir = plugin_dir;
    if (mesh_loader_plugin_name == "default") {
        mesh_loader_plugin_name = selectDefaultPreProcessorFromMeshFilename(mesh_filename);
    }
    mp_rootprint("Importing mesh: %s\n", mesh_filename.c_str());
    mp_rootprint("Using plugin: %s\n", mesh_loader_plugin_name.c_str());
    auto pp = getMeshLoader(dir, mesh_loader_plugin_name);
    auto mesh = pp->load(mp.getCommunicator(), mesh_filename);
    if (verbose) {
        auto tinf_mesh = TinfMesh(mesh);
        printf("Printing verbose mesh: \n%s", tinf_mesh.to_string().c_str());
    }
    return mesh;
}
std::shared_ptr<MeshInterface> importMesh(std::string mesh_filename,
                                          const Parfait::CommandLineMenu& m,
                                          const MessagePasser& mp) {
    TRACER_PROFILE_FUNCTION
    std::string dir = inf::getPluginDir();
    if (m.has(Alias::plugindir())) {
        dir = m.get(Alias::plugindir());
    }
    auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
    if (mesh_loader_plugin_name == "default") {
        mesh_loader_plugin_name = selectDefaultPreProcessorFromMeshFilename(mesh_filename);
    }
    bool verbose = false;
    if (m.has("verbose")) verbose = true;
    return importMesh(mp, mesh_filename, mesh_loader_plugin_name, verbose, dir);
}
std::shared_ptr<MeshInterface> importMesh(const Parfait::CommandLineMenu& m,
                                          const MessagePasser& mp) {
    TRACER_PROFILE_FUNCTION
    std::string dir = inf::getPluginDir();
    if (m.has(Alias::plugindir())) {
        dir = m.get(Alias::plugindir());
    }
    auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
    auto mesh_filename = m.get(Alias::mesh());
    if (mesh_loader_plugin_name == "default") {
        mesh_loader_plugin_name = selectDefaultPreProcessorFromMeshFilename(mesh_filename);
    }
    bool verbose = false;
    if (m.has("verbose")) verbose = true;
    return importMesh(mp, mesh_filename, mesh_loader_plugin_name, verbose, dir);
}
void exportMeshAndFields(std::string output_filename,
                         const std::shared_ptr<inf::MeshInterface>& mesh,
                         const Parfait::CommandLineMenu& m,
                         const MessagePasser& mp,
                         const std::vector<std::shared_ptr<inf::FieldInterface>>& fields) {
    auto name = m.get(Alias::postprocessor());
    if (name == "default") {
        name = selectDefaultPostProcessorFromMeshFilename(output_filename);
    }
    auto dir = m.get(Alias::plugindir());
    auto factory = inf::getVizFactory(dir, name);
    mp_rootprint("Writing file with basename: %s\n", output_filename.c_str());
    mp_rootprint("Using plugin: %s\n", name.c_str());
    auto viz = factory->create(output_filename, mesh, mp.getCommunicator());
    for (auto& f : fields) viz->addField(f);
    viz->visualize();
}
void appendFieldsFromSnap(MessagePasser mp,
                          std::string snap_filename,
                          const MeshInterface& mesh,
                          const Parfait::CommandLineMenu& m,
                          std::vector<std::shared_ptr<FieldInterface>>& fields) {
    TRACER_PROFILE_FUNCTION
    Snap snap(mp.getCommunicator());
    snap.addMeshTopologies(mesh);
    mp_rootprint("Loading snap file: %s\n", snap_filename.c_str());
    snap.load(snap_filename);
    auto field_names = getOutputFieldNames(m, snap.availableFields());
    for (auto& name : field_names) {
        mp_rootprint("Extracting field: %s\n", name.c_str());
        fields.emplace_back(snap.retrieve(name));
    }
}

void appendFieldsFromSolb(MessagePasser mp,
                          std::string filename,
                          const MeshInterface& mesh,
                          const Parfait::CommandLineMenu& m,
                          std::vector<std::shared_ptr<FieldInterface>>& fields) {
    TRACER_PROFILE_FUNCTION
    auto refine_field_loader = getFieldLoader(m.get(Alias::plugindir()), "RefinePlugins");
    refine_field_loader->load(filename, mp.getCommunicator(), mesh);

    auto field_names = getOutputFieldNames(m, refine_field_loader->availableFields());
    for (auto& name : field_names) {
        mp_rootprint("Extracting field: %s\n", name.c_str());
        fields.emplace_back(refine_field_loader->retrieve(name));
    }
}
std::vector<std::shared_ptr<FieldInterface>> importFields(MessagePasser mp,
                                                          std::shared_ptr<MeshInterface> mesh,
                                                          const Parfait::CommandLineMenu& m,
                                                          std::string filename) {
    TRACER_PROFILE_FUNCTION
    std::vector<std::shared_ptr<FieldInterface>> fields;
    if (filename.empty()) return fields;
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "solb") {
        appendFieldsFromSolb(mp, filename, *mesh, m, fields);
    } else {
        appendFieldsFromSnap(mp, filename, *mesh, m, fields);
    }
    return fields;
}
std::vector<std::shared_ptr<FieldInterface>> importFields(MessagePasser mp,
                                                          std::shared_ptr<MeshInterface> mesh,
                                                          const Parfait::CommandLineMenu& m) {
    TRACER_PROFILE_FUNCTION
    auto snap_alias = Alias::snap();
    auto solb_alias = "solb";
    std::string filename;
    if (m.has(snap_alias)) {
        filename = m.get(snap_alias);
    }
    if (m.has(solb_alias)) {
        filename = m.get(solb_alias);
    }
    return importFields(mp, mesh, m, filename);
}
std::string selectDefaultPreProcessorFromMeshFilename(std::string filename) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "csm") {
        return "RefinePluginsEGADS";
    }
    if (extension == "egads") {
        return "RefinePluginsEGADS";
    }
    if (extension == "meshb") {
        return "RefinePlugins";
    }
    if (extension == "g" or extension == "grd") {
        return "regularsolve";
    }
    return "NC_PreProcessor";
}
std::string selectDefaultPostProcessorFromMeshFilename(std::string filename) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "meshb") {
        return "RefinePlugins";
    }
    if (extension == "solb") {
        return "RefinePlugins";
    }
    return "ParfaitViz";
}
bool atNodes(Parfait::CommandLineMenu m) {
    if (m.has("at")) {
        auto where = m.get("at");
        where = Parfait::StringTools::tolower(where);
        if (where == "node" or where == "nodes") return true;
    }
    return false;
}
bool atCells(Parfait::CommandLineMenu m) {
    if (m.has("at")) {
        auto where = m.get("at");
        where = Parfait::StringTools::tolower(where);
        if (where == "cell" or where == "cells") return true;
    }
    return false;
}
}
