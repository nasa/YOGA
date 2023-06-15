#pragma once
#include <string>
#include <memory>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include "MeshLoader.h"
#include "PluginLocator.h"
#include "VizInterface.h"
#include "VizFactory.h"
#include "VectorFieldAdapter.h"
#include "FilterFactory.h"

namespace inf {
namespace shortcut {
    std::shared_ptr<inf::MeshInterface> loadMesh(MessagePasser mp,
                                                 std::string filename,
                                                 std::string plugin_name = "default");

    std::vector<std::shared_ptr<inf::FieldInterface>> loadFields(MessagePasser mp,
                                                                 std::string filename,
                                                                 const MeshInterface& mesh);

    void visualize(std::string filename,
                   MessagePasser mp,
                   std::shared_ptr<MeshInterface> mesh,
                   const std::vector<std::shared_ptr<inf::FieldInterface>>& fields = {},
                   std::string plugin_name = "default");

    std::shared_ptr<inf::MeshInterface> shard(MessagePasser mp, const inf::MeshInterface& mesh);

    auto filterBySurfaceTags(MessagePasser mp,
                             std::shared_ptr<inf::MeshInterface> mesh,
                             const std::vector<int>& tags,
                             const std::vector<std::shared_ptr<inf::FieldInterface>>& fields)
        -> std::tuple<std::shared_ptr<inf::MeshInterface>,
                      std::vector<std::shared_ptr<inf::FieldInterface>>>;

    void visualize_volume(std::string filename,
                          MessagePasser mp,
                          std::shared_ptr<MeshInterface> mesh,
                          const std::vector<std::shared_ptr<inf::FieldInterface>>& fields);

    void visualize_surface(std::string filename,
                           MessagePasser mp,
                           std::shared_ptr<MeshInterface> mesh,
                           const std::vector<std::shared_ptr<inf::FieldInterface>>& fields = {});

    void visualize_surface_at_cells(std::string filename,
                                    MessagePasser mp,
                                    std::shared_ptr<MeshInterface> mesh,
                                    const std::vector<std::vector<double>>& fields,
                                    const std::vector<std::string>& field_names);

    void visualize_surface_tags(std::string filename,
                                MessagePasser mp,
                                std::shared_ptr<MeshInterface> mesh,
                                const std::vector<int>& tags,
                                const std::vector<std::shared_ptr<inf::FieldInterface>>& field);
    void visualize_surface_tags(std::string filename,
                                MessagePasser mp,
                                std::shared_ptr<MeshInterface> mesh,
                                const std::vector<int>& tags,
                                const std::vector<std::vector<double>>& cell_fields,
                                const std::vector<std::string>& field_names);

    void visualize_with_node_and_cell_ids(std::string filename,
                                          MessagePasser mp,
                                          std::shared_ptr<MeshInterface> mesh);

    void visualize_surface_cells(std::string filename,
                                 MessagePasser mp,
                                 std::shared_ptr<MeshInterface> mesh,
                                 const std::vector<std::vector<double>>& cell_fields,
                                 const std::vector<std::string>& field_names);

    void visualize(std::string filename,
                   MessagePasser mp,
                   std::shared_ptr<MeshInterface> mesh,
                   std::string association,
                   const std::vector<std::vector<double>>& fields,
                   const std::vector<std::string>& field_names,
                   std::string plugin_name = "ParfaitViz");

    void visualize(std::string filename,
                   MessagePasser mp,
                   std::shared_ptr<MeshInterface> mesh,
                   std::string association,
                   const std::vector<std::vector<double>>& fields,
                   std::string plugin_name = "default");

    void visualize_partitions(std::string filename,
                              MessagePasser mp,
                              std::shared_ptr<MeshInterface> mesh,
                              std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields = {},
                              std::string plugin_name = "default");

    void visualize_volume_partitions(
        std::string filename,
        MessagePasser mp,
        std::shared_ptr<MeshInterface> mesh,
        std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields = {},
        std::string plugin_name = "default");

    void visualize_surface_partitions(
        std::string filename,
        MessagePasser mp,
        std::shared_ptr<MeshInterface> mesh,
        std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields = {},
        std::string plugin_name = "default");

    void visualize_at_nodes(std::string filename,
                            MessagePasser mp,
                            std::shared_ptr<MeshInterface> mesh,
                            const std::vector<std::vector<double>>& fields,
                            const std::vector<std::string>& field_names,
                            std::string plugin_name = "ParfaitViz");

    void visualize_volume_at_cells(std::string filename,
                                   MessagePasser mp,
                                   std::shared_ptr<MeshInterface> mesh,
                                   const std::vector<std::vector<double>>& fields,
                                   const std::vector<std::string>& field_names,
                                   std::string plugin_name = "ParfaitViz");

    void visualize_at_cells(std::string filename,
                            MessagePasser mp,
                            std::shared_ptr<MeshInterface> mesh,
                            const std::vector<std::vector<double>>& fields,
                            const std::vector<std::string>& field_names,
                            std::string plugin_name = "ParfaitViz");

    void visualize_at_nodes(std::string filename,
                            MessagePasser mp,
                            std::shared_ptr<MeshInterface> mesh,
                            const std::vector<std::vector<double>>& fields);

    void visualize_at_cells(std::string filename,
                            MessagePasser mp,
                            std::shared_ptr<MeshInterface> mesh,
                            const std::vector<std::vector<double>>& fields);

    void visualize_line_sample(
        std::string filename,
        MessagePasser mp,
        std::shared_ptr<MeshInterface> mesh,
        Parfait::Point<double> a,
        Parfait::Point<double> b,
        const std::vector<std::shared_ptr<inf::FieldInterface>>& fields = {});
    void visualize_exploded(std::string filename,
                            MessagePasser mp,
                            std::shared_ptr<MeshInterface> mesh,
                            double scale = 0.8,
                            const std::vector<std::shared_ptr<inf::FieldInterface>>& fields = {},
                            std::string plugin_name = "default");

    std::shared_ptr<inf::VizInterface> loadViz(std::string output_filename,
                                               std::shared_ptr<MeshInterface> mesh,
                                               MessagePasser mp,
                                               std::string directory = "default",
                                               std::string plugin_name = "default");
}
}
