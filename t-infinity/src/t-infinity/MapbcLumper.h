#pragma once

namespace inf {

std::map<int, int> groupTagsByBoundaryConditionName(Parfait::BoundaryConditionMap bc) {
    std::map<int, int> new_tags;
    std::map<std::string, int> new_tag_for_name;
    int tag = 1;
    for (auto entry : bc) {
        auto name = entry.second.second;
        if (0 == new_tag_for_name.count(name)) new_tag_for_name[name] = tag++;
        new_tags[entry.first] = new_tag_for_name[name];
    }
    return new_tags;
}

Parfait::BoundaryConditionMap lumpBC(const Parfait::BoundaryConditionMap& bc,
                                     const std::map<int, int>& old_to_new) {
    Parfait::BoundaryConditionMap lumped;
    for (auto entry : bc) {
        auto name = entry.second.second;
        int tag = entry.first;
        int bc_num = entry.second.first;
        int new_tag = old_to_new.at(tag);
        lumped[new_tag] = {bc_num, name};
    }
    return lumped;
}

}
