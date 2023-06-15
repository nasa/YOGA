#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace inf {

class FieldAttributes {
  public:
    static inline std::string Association() { return "association"; }
    static inline std::string Node() { return "node"; }
    static inline std::string Cell() { return "cell"; }
    static inline std::string DataType() { return "DataType"; }
    static inline std::string Int32() { return "INT32"; }
    static inline std::string Int64() { return "INT64"; }
    static inline std::string Float32() { return "FLOAT32"; }
    static inline std::string Float64() { return "FLOAT64"; }
    static inline std::string name() { return "name"; }
    static inline std::string outputOrder() { return "OutputOrder"; }
    static inline std::string solutionTime() { return "SolutionTime"; }
    static inline std::string Unassigned() { return "UNASSIGNED"; }
};

class FieldInterface {
  public:
    inline FieldInterface() {
        attribute_dictionary["name"] = FieldAttributes::Unassigned();
        attribute_dictionary["association"] = FieldAttributes::Unassigned();
    }
    virtual ~FieldInterface() = default;
    virtual int size() const = 0;
    virtual int blockSize() const = 0;
    virtual void value(int entity_id, void* v) const = 0;

    inline virtual std::string name() const final { return attribute(FieldAttributes::name()); }

    inline virtual std::string association() const final {
        return attribute(FieldAttributes::Association());
    }

    std::vector<double> at(int entity_id) const {
        if (attribute(FieldAttributes::DataType()) != FieldAttributes::Float64())
            throw std::logic_error(".at only supports FLOAT64 entryType");
        std::vector<double> v(blockSize());
        value(entity_id, v.data());
        return v;
    }

    inline std::string attribute(std::string key) const {
        if (attribute_dictionary.count(key) == 1) {
            return attribute_dictionary.at(key);
        } else {
            return "";
        }
    }

    inline std::unordered_map<std::string, std::string> getAllAttributes() const {
        return attribute_dictionary;
    }

    double getDouble(int row, int col = 0) const {
        if (col >= blockSize()) {
            throw std::logic_error(".getDouble column out of range.");
        }
        std::vector<double> v(blockSize());
        value(row, v.data());
        return v[col];
    }

    double max() const {
        double norm = 0.0;
        int n = blockSize();
        std::vector<double> entry(n);
        for (int i = 0; i < size(); i++) {
            value(i, entry.data());
            for (double v : entry) {
                norm = std::max(norm, v);
            }
        }
        return norm;
    }

    double norm(int p) const {
        double norm = 0.0;
        int n = blockSize();
        std::vector<double> entry(n);
        for (int i = 0; i < size(); i++) {
            value(i, entry.data());
            for (double v : entry) {
                norm += std::pow(v, p);
            }
        }

        return std::pow(norm, 1. / p);
    }

  protected:
    inline void setAttribute(std::string key, std::string value) {
        std::string msg =
            "FieldInterface: [" + key + "] [" + value + "] keys/values cannot be empty";
        if (key.empty()) throw std::logic_error(msg);
        if (value.empty()) throw std::logic_error(msg);
        attribute_dictionary[key] = value;
    }

  private:
    std::unordered_map<std::string, std::string> attribute_dictionary;
};
}
