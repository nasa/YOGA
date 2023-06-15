#pragma once
#include <t-infinity/FieldExtractor.h>
#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>

namespace inf {
void visualizeFromScript(MessagePasser mp,
                         std::shared_ptr<MeshInterface> mesh,
                         std::shared_ptr<FieldExtractor> field_extractor,
                         const std::string& plugin_dir,
                         const std::string& script);
}
