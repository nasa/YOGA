#pragma once
#include <parfait/Throw.h>

#include <string>
#include <utility>
#include <vector>
#include <array>
#include <t-infinity/FieldInterface.h>

namespace inf {
class VectorFieldAdapter : public FieldInterface {
  public:
    VectorFieldAdapter(std::string name, std::string association, int entry_length, size_t size);

    template <typename T>
    VectorFieldAdapter(const std::string& name,
                       const std::string& association,
                       int entry_length,
                       const std::vector<T>& field)
        : entry_length(entry_length) {
        PARFAIT_ASSERT(field.size() % entry_length == 0,
                       "Field data does not evenly divide into entry_legnth.");
        input_field.reserve(field.size());
        for (const T& f : field) input_field.push_back(f);
        setAttributes(name, association);
    }

    template <typename T>
    VectorFieldAdapter(const std::string& name,
                       const std::string& association,
                       const std::vector<T>& field)
        : VectorFieldAdapter(name, association, 1, field) {}

    VectorFieldAdapter(std::string name,
                       std::string association,
                       int entry_length_in,
                       const double* field,
                       size_t size);

    int size() const override;
    int blockSize() const override;
    void value(int entity_id, void* v_in) const override;
    std::vector<double>& getVector();
    const std::vector<double>& getVector() const;
    void setAdapterAttribute(std::string key, std::string value);
    void setAdapterAttributes(const std::unordered_map<std::string, std::string>& attributes);

  private:
    int entry_length;
    std::vector<double> input_field;

    void setAttributes(std::string name, std::string association);
};
}
