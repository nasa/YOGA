#include "DataFrame.h"
#include <limits>

using namespace Parfait;

DataFrame DataFrame::select(const std::string& name, double range_start, double range_end) const {
    std::set<int> rows;
    for (size_t i = 0; i < _data.at(name).size(); i++) {
        auto x = _data.at(name)[i];
        if (x >= range_start and x < range_end) rows.insert(i);
    }
    DataFrame slice;
    for (auto s : column_names) {
        auto& col = slice[s];
        col.reserve(rows.size());
        for (auto row : rows) col.push_back(_data.at(s)[row]);
    }
    return slice;
}

std::vector<double> reorderVector(const std::vector<double>& vec, const std::vector<int>& old_to_new) {
    std::vector<double> out(vec.size());
    for (int old_index = 0; old_index < int(old_to_new.size()); old_index++) {
        int new_index = old_to_new[old_index];
        out[new_index] = vec[old_index];
    }
    return out;
}

void DataFrame::sort(const std::string& column_name) {
    auto& data = (*this)[column_name];
    std::vector<std::pair<double, int>> pairs;

    for (int i = 0; i < int(data.size()); i++) {
        pairs.push_back({data[i], i});
    }

    std::sort(pairs.begin(), pairs.end());

    std::vector<int> old_to_new(pairs.size());
    for (int new_index = 0; new_index < int(old_to_new.size()); new_index++) {
        int old_index = pairs[new_index].second;
        old_to_new[old_index] = new_index;
    }
    pairs.clear();
    pairs.shrink_to_fit();

    for (auto& c : columns()) {
        auto& col = (*this)[c];
        col = reorderVector(col, old_to_new);
    }
}
double DataFrame::min(const std::string& column_name) const {
    double min = std::numeric_limits<double>::max();
    auto& data = (*this)[column_name];
    for (auto d : data) {
        min = std::min(min, d);
    }
    return min;
}
double DataFrame::max(const std::string& column_name) const {
    double max = std::numeric_limits<double>::lowest();
    auto& data = (*this)[column_name];
    for (auto d : data) {
        max = std::max(max, d);
    }
    return max;
}
double DataFrame::average(const std::string& column_name) const {
    double avg = 0.0;
    auto& data = (*this)[column_name];
    for (auto d : data) {
        avg += d;
    }
    return avg / double(data.size());
}
