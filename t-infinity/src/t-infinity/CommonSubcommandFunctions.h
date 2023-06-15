#pragma once
#include <string>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include <parfait/CommandLineMenu.h>
#include <t-infinity/Snap.h>

namespace inf {
inline std::vector<int> getTags(const Parfait::CommandLineMenu& m) {
    return Parfait::StringTools::parseIntegerList(m.get("tags"));
}
inline std::vector<std::string> getBCNames(const Parfait::CommandLineMenu& m) {
    return Parfait::StringTools::split(m.get("bc-names"), ",");
}
std::vector<std::string> getOutputFieldNames(const Parfait::CommandLineMenu& m,
                                             const inf::Snap& snap);
std::string selectDefaultPreProcessorFromMeshFilename(std::string filename);
std::string selectDefaultPostProcessorFromMeshFilename(std::string filename);
std::shared_ptr<MeshInterface> importMesh(MessagePasser mp,
                                          std::string mesh_filename,
                                          std::string mesh_loader_plugin_name = "default",
                                          bool verbose = false,
                                          std::string plugin_dir = "");
std::shared_ptr<MeshInterface> importMesh(const Parfait::CommandLineMenu& m,
                                          const MessagePasser& mp);
std::shared_ptr<MeshInterface> importMesh(std::string mesh_filename,
                                          const Parfait::CommandLineMenu& m,
                                          const MessagePasser& mp);
void exportMeshAndFields(std::string mesh_filename,
                         const std::shared_ptr<inf::MeshInterface>& mesh,
                         const Parfait::CommandLineMenu& m,
                         const MessagePasser& mp,
                         const std::vector<std::shared_ptr<inf::FieldInterface>>& fields = {});
void appendFieldsFromSnap(MessagePasser mp,
                          std::string snap_filename,
                          const MeshInterface& mesh,
                          const Parfait::CommandLineMenu& m,
                          std::vector<std::shared_ptr<FieldInterface>>& fields);
void appendFieldsFromSolb(MessagePasser mp,
                          std::string filename,
                          const MeshInterface& mesh,
                          const Parfait::CommandLineMenu& m,
                          std::vector<std::shared_ptr<FieldInterface>>& fields);
std::vector<std::shared_ptr<FieldInterface>> importFields(MessagePasser mp,
                                                          std::shared_ptr<MeshInterface> mesh,
                                                          const Parfait::CommandLineMenu& m);
std::vector<std::shared_ptr<FieldInterface>> importFields(MessagePasser mp,
                                                          std::shared_ptr<MeshInterface> mesh,
                                                          const Parfait::CommandLineMenu& m,
                                                          std::string filename);
bool atNodes(Parfait::CommandLineMenu m);

bool atCells(Parfait::CommandLineMenu m);

}
