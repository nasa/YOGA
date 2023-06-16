#pragma once
#include <t-infinity/MeshInterface.h>
#include <t-infinity/FieldExtractor.h>
#include <t-infinity/RecipeGenerator.h>
#include <t-infinity/VizInterface.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/PluginLoader.h>
#include "VizFactory.h"

namespace inf {

class Emeril {
  public:
    Emeril(std::shared_ptr<MeshInterface> m, std::shared_ptr<FieldExtractor> fe, MessagePasser mp)
        : mesh(m), field_extractor(fe), mp(mp) {}

    void make(const std::vector<inf::RecipeGenerator::Recipe>& recipes) {
        for (auto& r : recipes) make(r);
    }

    virtual ~Emeril() {}

  protected:
    std::shared_ptr<MeshInterface> mesh;
    std::shared_ptr<FieldExtractor> field_extractor;
    MessagePasser mp;
    virtual std::shared_ptr<inf::VizInterface> createVizObject(const std::string& plugin,
                                                               const std::string& plugin_directory,
                                                               const std::string& output_name,
                                                               std::shared_ptr<MeshInterface> m,
                                                               MessagePasser mp) {
        auto viz_plugin = inf::getVizFactory(plugin_directory, plugin);
        return viz_plugin->create(output_name, mesh, mp.getCommunicator());
    }

  private:
    void make(const RecipeGenerator::Recipe& recipe) {
        auto selector = std::make_shared<CompositeSelector>();
        for (auto& filter : recipe.filters) selector->add(filter->getCellSelector());
        CellIdFilter filter(mp.getCommunicator(), mesh, selector);
        auto viz = createVizObject(
            recipe.plugin, recipe.plugin_directory, recipe.filename, filter.getMesh(), mp);
        for (auto& name : recipe.fields) {
            auto field = field_extractor->extract(name);
            auto filtered_field = filter.apply(field);
            viz->addField(filtered_field);
        }
        viz->visualize();
    }
};

}
