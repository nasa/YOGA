#include "Dictionary.h"
#include <iomanip>
#include "ToString.h"
#include "StringTools.h"
#include "MapTools.h"
#include "JsonPrinters.h"

using namespace Parfait;
ValueEntry::ValueEntry(const std::string v, DictionaryEntry::TYPE v_type) : t(v_type), value(v) {}
DictionaryEntry::TYPE ValueEntry::type() const { return t; }
std::string ValueEntry::asString() const { return value; }
int ValueEntry::asInt() const {
    throwIfString();
    return StringTools::toInt(value);
}
bool ValueEntry::asBool() const {
    throwIfString();
    return "true" == value;
}
double ValueEntry::asDouble() const {
    throwIfString();
    return StringTools::toDouble(value);
}
void ValueEntry::throwIfString() const {
    if (String == t) throw std::logic_error("Cannot convert string to number");
}
DictionaryEntry::TYPE NullEntry::type() const { return Object; }
ValueEntry Dictionary::null() { return ValueEntry("null", TYPE::Null); }
Dictionary& Dictionary::operator=(const Dictionary& d) {
    Parfait::Dictionary copy(d);
    object_type = copy.object_type;
    object = copy.object;
    mapped_objects = copy.mapped_objects;
    indexed_objects = copy.indexed_objects;
    return (*this);
}
Dictionary& Dictionary::operator=(const std::string& s) {
    clear();
    object = std::make_shared<ValueEntry>(s, String);
    object_type = object->type();
    return *this;
}
Dictionary& Dictionary::operator=(const char* s) {
    clear();
    return this->operator=(std::string(s));
}
Dictionary& Dictionary::operator=(const int n) {
    clear();
    object = std::make_shared<ValueEntry>(std::to_string(n), Integer);
    object_type = object->type();
    return *this;
}
Dictionary& Dictionary::operator=(const double x) {
    clear();
    std::ostringstream stream;
    stream << std::setprecision(16) << x;
    std::string exponential_form = stream.str();
    object = std::make_shared<ValueEntry>(exponential_form, Double);
    object_type = object->type();
    return *this;
}
Dictionary& Dictionary::operator=(const std::vector<int> v) {
    clear();
    object_type = IntArray;
    int index = 0;
    for (auto value : v) indexed_objects[index++] = value;
    return *this;
}
Dictionary& Dictionary::operator=(const bool& b) {
    clear();
    std::string s = b ? "true" : "false";
    object = std::make_shared<ValueEntry>(s, Boolean);
    object_type = object->type();
    return *this;
}
Dictionary& Dictionary::operator=(const std::vector<bool> v) {
    clear();
    mapped_objects.clear();
    object_type = BoolArray;
    int index = 0;
    for (bool value : v) indexed_objects[index++] = value;
    return *this;
}
Dictionary& Dictionary::operator=(const std::vector<double> v) {
    clear();
    mapped_objects.clear();
    object_type = DoubleArray;
    int index = 0;
    for (auto value : v) indexed_objects[index++] = value;
    return *this;
}
Dictionary& Dictionary::operator=(const std::vector<std::string> v) {
    mapped_objects.clear();
    object_type = StringArray;
    int index = 0;
    for (auto value : v) indexed_objects[index++] = value;
    return *this;
}
Dictionary& Dictionary::operator=(const ValueEntry&& obj) {
    clear();
    object_type = obj.type();
    object = std::make_shared<ValueEntry>(obj);
    return *this;
}
bool Dictionary::isTrue(const std::string& key) const {
    if (not has(key)) return false;
    if (at(key).type() != Boolean) return false;
    return at(key).asBool();
}
int Dictionary::size() const { return std::max(mapped_objects.size(), indexed_objects.size()); }
int Dictionary::count(const std::string& key) const { return mapped_objects.count(key); }
DictionaryEntry::TYPE Dictionary::type() const { return object_type; }
Dictionary& Dictionary::operator[](const std::string& key) {
    if (mapped_objects.count(key) == 0) mapped_objects[key] = Dictionary();
    return mapped_objects[key];
}
Dictionary& Dictionary::operator[](int index) {
    object_type = ObjectArray;
    mapped_objects.clear();
    if (indexed_objects.count(index) == 0) indexed_objects[index] = Dictionary();
    return indexed_objects[index];
}
const Dictionary& Dictionary::operator[](const std::string& key) const { return at(key); }
const Dictionary& Dictionary::operator[](int index) const { return at(index); }
const Dictionary& Dictionary::at(const std::string& key) const {
    if (mapped_objects.count(key) == 0) {
        PARFAIT_THROW("Dictionary object has no key: " + key + "\nOptions include " +
                      to_string(MapTools::keys(mapped_objects)));
    }
    return mapped_objects.at(key);
}
const Dictionary& Dictionary::at(int index) const {
    if (indexed_objects.count(index) == 0) {
        PARFAIT_THROW("Dictionary has no indexed object at index: " + std::to_string(index));
    }
    return indexed_objects.at(index);
}
std::vector<std::string> Dictionary::keys() const {
    std::vector<std::string> k;
    for (auto& pair : mapped_objects) k.push_back(pair.first);
    return k;
}
std::string Dictionary::asString() const {
    PARFAIT_ASSERT(object != nullptr, "object is null");
    return object->asString();
}
int Dictionary::asInt() const { return object->asInt(); }
double Dictionary::asDouble() const { return object->asDouble(); }
bool Dictionary::asBool() const { return object->asBool(); }
std::vector<int> Dictionary::asInts() const {
    if (IntArray == object_type or DoubleArray == object_type or ObjectArray == object_type) {
        std::vector<int> v;
        for (auto& pair : indexed_objects) v.push_back(pair.second.asInt());
        return v;
    } else if (Integer == object_type) {
        return {asInt()};
    }
    throwCantConvertToType(IntArray);
    return {};
}
std::vector<double> Dictionary::asDoubles() const {
    if (DoubleArray == object_type or IntArray == object_type or ObjectArray == object_type) {
        std::vector<double> v;
        for (auto& pair : indexed_objects) v.push_back(pair.second.asDouble());
        return v;
    } else if (Integer == object_type) {
        return {asDouble()};
    } else if (Double == object_type) {
        return {asDouble()};
    }
    throwCantConvertToType(DoubleArray);
    return {};
}
std::vector<std::string> Dictionary::asStrings() const {
    if (StringArray == object_type) {
        std::vector<std::string> v;
        for (auto& pair : indexed_objects) v.push_back(pair.second.asString());
        return v;
    } else if (String == object_type) {
        return {asString()};
    }
    throwCantConvertToType(StringArray);
    return {};
}
std::vector<bool> Dictionary::asBools() const {
    if (BoolArray == object_type) {
        std::vector<bool> v;
        for (auto& pair : indexed_objects) v.push_back(pair.second.asBool());
        return v;
    } else if (Boolean == object_type) {
        return {asBool()};
    }
    throwCantConvertToType(BoolArray);
    return {};
}
std::vector<Dictionary> Dictionary::asObjects() const {
    if (ObjectArray == object_type) {
        std::vector<Dictionary> v;
        for (auto& pair : indexed_objects) {
            v.push_back(pair.second);
        }
        return v;
    } else if (Object == object_type) {
        return {*this};
    }
    throwCantConvertToType(ObjectArray);
    return {};
}
bool Dictionary::isValueObject() const {
    return Integer == object_type or Double == object_type or String == object_type or Boolean == object_type or
           Null == object_type;
}
bool Dictionary::isMappedObject() const { return not mapped_objects.empty(); }
bool Dictionary::isArrayObject() const {
    return IntArray == object_type or DoubleArray == object_type or StringArray == object_type or
           ObjectArray == object_type or BoolArray == object_type;
}
std::string Dictionary::dump() const {
    Json::CompactPrinter compact_printer;
    return compact_printer.dump(*this);
}
std::string Dictionary::dump(int indent) const {
    Json::PrettyPrinter pretty_printer(indent);
    return pretty_printer.dump(*this);
}
Dictionary Dictionary::overrideEntries(const Dictionary& config) const {
    auto updated = *this;
    auto& self = *this;
    for (const auto& key : self.keys()) {
        if (self.at(key).isMappedObject() and config.count(key) != 0) {
            updated[key] = (self.at(key)).overrideEntries(config.at(key));
        } else if (config.count(key) == 0) {
            updated[key] = self.at(key);
        } else {
            updated[key] = config.at(key);
        }
    }
    return updated;
}
void Dictionary::clear() {
    object_type = Null;
    object.reset();
    mapped_objects.clear();
    indexed_objects.clear();
}
void Dictionary::erase(const std::string& key) {
    if (mapped_objects.count(key) == 1) {
        mapped_objects.erase(key);
    }
}
