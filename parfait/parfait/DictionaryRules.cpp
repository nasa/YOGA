#include "DictionaryRules.h"
#include "Checkpoint.h"

#define RETURN_IF_FALSE(x)                   \
    s = x;                                   \
    if (not s) {                             \
        s.message = help + "\n" + s.message; \
        return s;                            \
    }

Parfait::Rules::Status::Status() : value(true) {}
Parfait::Rules::Status::Status(bool b) : value(b) {}
Parfait::Rules::Status::Status(bool b, std::string m) : value(b), message(std::move(m)) {}
Parfait::Rules::Status::operator std::string() const { return message; }
Parfait::Rules::Status::operator bool() const { return value; }
Parfait::Rules& Parfait::Rules::addEnum(std::string e) {
    enums.insert(e);
    return *this;
}
Parfait::Rules& Parfait::Rules::addOption(std::string e) {
    enums.insert(e);
    return *this;
}
Parfait::Rules& Parfait::Rules::addOneOf(const Parfait::Rules& r) {
    one_of.push_back(r);
    return *this;
}
Parfait::Rules& Parfait::Rules::disallowAdditionalFields() {
    allow_additional_properties = false;
    return *this;
}
Parfait::Rules& Parfait::Rules::requireField(std::string key) {
    required_keys.insert(key);
    return *this;
}
Parfait::Rules& Parfait::Rules::requireType(std::string s) {
    auto t = TypeFromString(s);
    required_type = t;
    return *this;
}
Parfait::Rules& Parfait::Rules::requireType(Parfait::Rules::Type t) {
    required_type = t;
    return *this;
}
Parfait::Rules& Parfait::Rules::requireMinimum(double m) {
    minimum = m;
    return *this;
}
Parfait::Rules& Parfait::Rules::requireMaximum(double m) {
    maximum = m;
    return *this;
}
Parfait::Rules& Parfait::Rules::requirePositiveDouble() {
    requireType("number");
    requireMinimum(0.0);
    return *this;
}
Parfait::Rules& Parfait::Rules::requireIs3DPoint() {
    requireType("array");
    requireMinimumLength(3);
    requireMaximumLength(3);
    return *this;
}
Parfait::Rules& Parfait::Rules::requireMinimumLength(int m) {
    minimum_length = m;
    return *this;
}
Parfait::Rules& Parfait::Rules::requireMaximumLength(int m) {
    maximum_length = m;
    return *this;
}
Parfait::Rules& Parfait::Rules::addCustomHelpMessage(std::string h) {
    help = h;
    return *this;
}
Parfait::Rules& Parfait::Rules::operator[](std::string s) { return properties[s]; }
Parfait::Rules& Parfait::Rules::at(std::string s) { return properties.at(s); }
const Parfait::Rules& Parfait::Rules::at(std::string s) const { return properties.at(s); }
const Parfait::Rules& Parfait::Rules::operator[](std::string s) const { return properties.at(s); }
Parfait::Rules::Status Parfait::Rules::check(const Parfait::Dictionary& dict) const {
    Status s;
    if (not one_of.empty()) return checkOneOf(dict);

    RETURN_IF_FALSE(isRequiredType(dict));
    RETURN_IF_FALSE(isRequiredValue(dict));
    RETURN_IF_FALSE(isWithinNumericBounds(dict));
    RETURN_IF_FALSE(isWithinArrayLengthBounds(dict));
    RETURN_IF_FALSE(hasAllRequiredKeys(dict));
    RETURN_IF_FALSE(checkAdditionalProperties(dict));
    RETURN_IF_FALSE(checkChildren(dict));
    RETURN_IF_FALSE(isEnum(dict));
    return true;
}
std::string Parfait::Rules::to_string() const {
    std::string s;
    if (not one_of.empty()) {
        s += "one of: \n";
        for (auto r : one_of) {
            s += r.to_string() + "\n";
        }
        return s;
    }

    if (required_type != unset) {
        s += "type: " + typeToString(required_type) + "\n";
    }

    if (not properties.empty()) {
        s += "properties:\n";
        for (auto p : properties) {
            s += " " + p.first + ":" + p.second.to_string() + "\n";
        }
        s += "\n";
    }

    if (not required_keys.empty()) {
        s += "required: ";
        for (auto k : required_keys) {
            s += " " + k;
        }
        s += "\n";
    }
    s += "additionalPropertires:" + std::string(allow_additional_properties ? "true" : "false");
    return s;
}
std::string Parfait::Rules::typeToString(Parfait::Rules::Type t) {
    switch (t) {
        case string:
            return "string";
        case number:
            return "number";
        case integer:
            return "integer";
        case boolean:
            return "boolean";
        case array:
            return "array";
        case object:
            return "object";
        case unset:
            return "unset";
        default:
            PARFAIT_THROW("encountered unexpected type");
    }
}
Parfait::Rules::Type Parfait::Rules::TypeFromString(std::string t) {
    t = Parfait::StringTools::tolower(t);
    if ("string" == t) {
        return string;
    }
    if ("number" == t) {
        return number;
    }
    if ("integer" == t) {
        return integer;
    }
    if ("bool" == t) {
        return boolean;
    }
    if ("boolean" == t) {
        return boolean;
    }
    if ("array" == t) {
        return array;
    }
    if ("object" == t) {
        return object;
    }
    if ("unset" == t) {
        return unset;
    }
    PARFAIT_THROW("encountered unexpected type " + t);
}
Parfait::Rules::Status Parfait::Rules::isEnum(const Parfait::Dictionary& dict) const {
    if (enums.empty()) return true;
    if (enums.count(dict.asString())) return true;
    Status s(false);
    s.message = dict.asString() + " is not a valid option: [";
    s.message += Parfait::to_string(enums, ",");
    s.message += "]";
    return s;
}
Parfait::Rules::Status Parfait::Rules::checkAdditionalProperties(const Parfait::Dictionary& dict) const {
    if (allow_additional_properties) return true;
    for (auto k : dict.keys()) {
        if (required_keys.count(k) == 0 and properties.count(k) == 0) {
            return {false, "disallowed additional property found: " + k};
        }
    }
    return true;
}
Parfait::Rules::Status Parfait::Rules::hasAllRequiredKeys(const Parfait::Dictionary& dict) const {
    for (auto k : required_keys) {
        if (not dict.has(k)) return {false, "missing required key: " + k};
    }
    return true;
}
Parfait::Rules::Status Parfait::Rules::isRequiredValue(const Parfait::Dictionary& dict) const {
    if (check_required_value == false) return true;
    if (isSame(dict.type(), boolean)) {
        return required_value_bool == dict.asBool();
    }
    if (isSame(dict.type(), number)) {
        return required_value_number == dict.asDouble();
    }
    if (isSame(dict.type(), string)) {
        return required_value_string == dict.asString();
    }
    return true;
}
Parfait::Rules::Status Parfait::Rules::isRequiredType(const Parfait::Dictionary& dict) const {
    if (required_type == unset) return true;
    auto s = isSame(dict.type(), required_type);
    if (not s) {
        s.message = "type must be " + typeToString(required_type) + " but is " + dict.typeString();
    }
    return s;
}
Parfait::Rules::Status Parfait::Rules::isWithinNumericBounds(const Parfait::Dictionary& dict) const {
    if (not isNumber(dict.type())) return true;
    double d = dict.asDouble();
    Status s;
    if (d < minimum) {
        s.value = false;
        s.message = std::to_string(d) + " is below minimum value " + std::to_string(minimum);
    }
    if (d > maximum) {
        s.value = false;
        s.message = std::to_string(d) + " is above maximum value " + std::to_string(maximum);
    }
    return s;
}
Parfait::Rules::Status Parfait::Rules::isWithinArrayLengthBounds(const Parfait::Dictionary& dict) const {
    if (not isArray(dict.type())) return true;
    int d = dict.size();
    Status s;
    if (d < minimum_length) {
        s.value = false;
        s.message = std::to_string(d) + " is below minimum length " + std::to_string(minimum_length);
    }
    if (d > maximum_length) {
        s.value = false;
        s.message = std::to_string(d) + " is above maximum length " + std::to_string(maximum_length);
    }
    return s;
}
bool Parfait::Rules::isNumber(Parfait::DictionaryEntry::TYPE t) const {
    using namespace Parfait::VectorTools;
    using T = Parfait::DictionaryEntry::TYPE;
    return oneOf(t, {T::Double, T::Integer});
}
bool Parfait::Rules::isArray(Parfait::DictionaryEntry::TYPE t) const {
    using namespace Parfait::VectorTools;
    using T = Parfait::DictionaryEntry::TYPE;
    return oneOf(t, {T::BoolArray, T::DoubleArray, T::IntArray, T::ObjectArray, T::StringArray});
}
Parfait::Rules::Status Parfait::Rules::isSame(Parfait::DictionaryEntry::TYPE t1, Parfait::Rules::Type t2) const {
    using T = Parfait::DictionaryEntry::TYPE;
    using namespace Parfait::VectorTools;
    switch (t2) {
        case string:
            return t1 == T::String;
        case integer:
        case number:
            return isNumber(t1);
        case boolean:
            return t1 == T::Boolean;
        case array:
            return isArray(t1);
        case object:
            return t1 == T::Object;
        default:
            PARFAIT_THROW("Should not be here");
    }
}
Parfait::Rules::Status Parfait::Rules::checkChildren(const Parfait::Dictionary& dict) const {
    Status s;
    for (auto k : dict.keys()) {
        if (properties.count(k)) {
            RETURN_IF_FALSE(properties.at(k).check(dict.at(k)));
        }
    }
    return true;
}
Parfait::Rules::Status Parfait::Rules::checkOneOf(const Parfait::Dictionary& dict) const {
    std::vector<Status> one_of_status;
    for (auto r : one_of) {
        auto s = r.check(dict);
        if (s) {
            return true;
        }
        one_of_status.push_back(s);
    }
    Status s(false, "is not one of: ");
    for (auto t : one_of_status) {
        s.message += "\n  " + t.message;
    }
    return s;
}
Parfait::Rules& Parfait::Rules::requireTrue() {
    check_required_value = true;
    required_value_bool = true;
    return requireType(Rules::boolean);
}
Parfait::Rules& Parfait::Rules::requireFalse() {
    check_required_value = true;
    required_value_bool = false;
    return requireType(Rules::boolean);
}
Parfait::Rules& Parfait::Rules::requireEquals(bool m) {
    if (m)
        return requireTrue();
    else
        return requireFalse();
}
Parfait::Rules& Parfait::Rules::requireEquals(double m) {
    check_required_value = true;
    required_value_number = m;
    return requireType(Rules::number);
}
Parfait::Rules& Parfait::Rules::requireEquals(std::string m) {
    check_required_value = true;
    required_value_string = m;
    return requireType(Rules::string);
}
Parfait::Rules& Parfait::Rules::requireEquals(const char* m) {
    check_required_value = true;
    required_value_string = m;
    return requireType(Rules::string);
}
