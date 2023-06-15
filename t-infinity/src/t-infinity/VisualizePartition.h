#pragma once
#include <string>
#include <t-infinity/MeshInterface.h>

#include "VizFactory.h"

namespace inf {
void visualizePartition(std::string filename_base,
                        const MeshInterface& mesh,
                        const std::vector<std::shared_ptr<inf::FieldInterface>>& fields,
                        std::string viz_plugin_name = "ParfaitViz",
                        std::string plugin_directory = "");
}
