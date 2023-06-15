#include "ScriptVisualization.h"
#include <t-infinity/Emeril.h>

namespace inf {

std::vector<RecipeGenerator::Recipe> tryParse(const std::string& script) {
    std::vector<RecipeGenerator::Recipe> recipes;
    try {
        recipes = RecipeGenerator::parse(script);
    } catch (std::exception& e) {
        printf("Warning, could not parse script: %s\n", e.what());
    }
    return recipes;
}

void tryMake(const MessagePasser& mp,
             const std::shared_ptr<MeshInterface>& mesh,
             const std::shared_ptr<FieldExtractor>& field_extractor,
             const std::vector<RecipeGenerator::Recipe>& recipes,
             const std::string& plugin_dir) {
    try {
        Emeril emeril(mesh, field_extractor, mp);
        emeril.make(recipes);
    } catch (std::exception& e) {
        printf("Warning, could not make recipe: %s\n", e.what());
    }
}

void visualizeFromScript(MessagePasser mp,
                         std::shared_ptr<MeshInterface> mesh,
                         std::shared_ptr<FieldExtractor> field_extractor,
                         const std::string& plugin_dir,
                         const std::string& script) {
    auto recipes = tryParse(script);
    tryMake(mp, mesh, field_extractor, recipes, plugin_dir);
}
}