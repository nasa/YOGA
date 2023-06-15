#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <array>
#include "VectorTools.h"
#include "Throw.h"

namespace Parfait {

class DataFrame {
  public:
    using MappedColumns = std::map<std::string, std::vector<double>>;

    DataFrame() = default;
    inline explicit DataFrame(MappedColumns data_frame) : _data(std::move(data_frame)) {
        for (const auto& p : _data) column_names.push_back(p.first);
    }

    DataFrame select(const std::string& name, double range_start, double range_end) const;
    void sort(const std::string& column_name);

    inline std::vector<std::string> columns() const { return column_names; }
    bool has(const std::string& column_name) const { return _data.count(column_name); }
    double min(const std::string& column_name) const;
    double max(const std::string& column_name) const;
    double average(const std::string& column_name) const;
    inline const std::vector<double>& operator[](const std::string& name) const { return _data.at(name); }
    inline std::vector<double>& operator[](const std::string& name) {
        if (not VectorTools::isIn(column_names, name)) column_names.push_back(name);
        return _data[name];
    }

    inline std::array<int, 2> shape() const {
        throwIfNotEqualDimensions();
        if (_data.size() == 0) return {0, 0};
        return {int(_data.begin()->second.size()), int(_data.size())};
    }

    inline std::map<int, DataFrame> groupBy(const std::string& col) const {
        auto categories = extractIntegerCategoriesFromCol(col);
        std::map<int, DataFrame> categorized;
        for (int c : categories) categorized[c] = select(col, c, c + 1);
        return categorized;
    }

  private:
    std::vector<std::string> column_names;
    MappedColumns _data;

    inline bool areAllDimensionsEqual() const {
        if (_data.empty()) return true;
        long length = _data.begin()->second.size();
        for (const auto& pair : _data) {
            if (long(pair.second.size()) != length) return false;
        }
        return true;
    }

    inline void throwIfNotEqualDimensions() const {
        if (not areAllDimensionsEqual()) {
            std::string m = "DataFrame: The dimensions of all columns are not equal\n";
            for (size_t i = 0; i < column_names.size(); i++) {
                m += "[" + column_names[i] + "] length: " + std::to_string(_data.at(column_names[i]).size()) + "\n";
            }
        }
    }

    inline std::set<int> extractIntegerCategoriesFromCol(const std::string& col) const {
        std::set<int> categories;
        for (auto n : _data.at(col)) categories.insert(int(n));
        return categories;
    }
};
}
