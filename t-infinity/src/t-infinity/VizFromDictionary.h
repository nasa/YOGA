#pragma once
#include "Shortcuts.h"
#include "PluginLoader.h"
#include <MessagePasser/MessagePasser.h>
#include "MeshInterface.h"
#include "FieldInterface.h"
#include <parfait/Dictionary.h>
#include <memory>
#include <parfait/DictionaryRules.h>

namespace inf {

void visualizeFromDictionary(MessagePasser mp,
                             std::shared_ptr<inf::MeshInterface> mesh,
                             std::vector<std::shared_ptr<inf::FieldInterface>> fields,
                             const Parfait::Dictionary& dict);

Parfait::Rules getVisualizeFromDictionaryRules();

template <typename Filter>
void visualizeFromFilter(MessagePasser mp,
                         Filter filter,
                         const std::vector<std::shared_ptr<FieldInterface>>& fields,
                         const std::vector<std::string>& output_names) {
    auto filtered_mesh = filter.getMesh();
    std::vector<std::shared_ptr<inf::FieldInterface>> filtered_fields;

    for (auto f : fields) {
        filtered_fields.push_back(filter.apply(f));
    }

    for (auto output_base : output_names) {
        auto viz = shortcut::loadViz(output_base, filtered_mesh, mp, getPluginDir(), "ParfaitViz");
        for (auto& f : filtered_fields) viz->addField(f);
        viz->visualize();
    }
}

}