#include "VectorFieldAdapter.h"
#include <parfait/VectorTools.h>
inf::VectorFieldAdapter::VectorFieldAdapter(std::string name,
                                            std::string association,
                                            int entry_length_in,
                                            size_t size)
    : entry_length(entry_length_in), input_field(size * entry_length_in) {
    setAttributes(name, association);
}
inf::VectorFieldAdapter::VectorFieldAdapter(std::string name,
                                            std::string association,
                                            int entry_length_in,
                                            const double* field,
                                            size_t size)
    : entry_length(entry_length_in), input_field(field, field + size * size_t(entry_length)) {
    setAttributes(name, association);
}

int inf::VectorFieldAdapter::size() const { return input_field.size() / entry_length; }
int inf::VectorFieldAdapter::blockSize() const { return entry_length; }
void inf::VectorFieldAdapter::value(int entity_id, void* v_in) const {
    auto* v = (double*)v_in;
    for (int i = 0; i < entry_length; i++) v[i] = input_field.at(entry_length * entity_id + i);
}
void inf::VectorFieldAdapter::setAttributes(std::string name, std::string association) {
    setAttribute(FieldAttributes::name(), name);
    setAttribute(FieldAttributes::Association(), association);
    setAttribute(FieldAttributes::DataType(), FieldAttributes::Float64());
}
std::vector<double>& inf::VectorFieldAdapter::getVector() { return input_field; }
const std::vector<double>& inf::VectorFieldAdapter::getVector() const { return input_field; }
void inf::VectorFieldAdapter::setAdapterAttribute(std::string key, std::string value) {
    FieldInterface::setAttribute(key, value);
}
void inf::VectorFieldAdapter::setAdapterAttributes(
    const std::unordered_map<std::string, std::string>& attributes) {
    for (auto& pair : attributes) {
        FieldInterface::setAttribute(pair.first, pair.second);
    }
}
