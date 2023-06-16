#pragma once
#include <set>
#include <map>
#include <limits>
#include "Dictionary.h"
#include "VectorTools.h"
#include "StringTools.h"
#include "ToString.h"

namespace Parfait {

class Rules {
  public:
    class Status {
      public:
        Status();
        Status(bool b);
        Status(bool b, std::string m);

        operator std::string() const;
        operator bool() const;

      public:
        bool value;
        std::string message;
    };

    enum Type { string, number, integer, boolean, array, object, unset };
    Rules& addEnum(std::string e);

    Rules& addOption(std::string e);
    Rules& addOneOf(const Rules& r);
    Rules& disallowAdditionalFields();
    Rules& requireField(std::string key);

    Rules& requireType(std::string s);

    Rules& requireTrue();
    Rules& requireFalse();

    Rules& requireEquals(double m);
    Rules& requireEquals(bool m);
    Rules& requireEquals(std::string s);
    Rules& requireEquals(const char* s);

    Rules& requireType(Type t);

    Rules& requireMinimum(double m);
    Rules& requireMaximum(double m);
    Rules& requireMinimumLength(int m);
    Rules& requireMaximumLength(int m);
    Rules& requireIs3DPoint();
    Rules& requirePositiveDouble();
    Rules& addCustomHelpMessage(std::string h);

    Rules& operator[](std::string s);
    Rules& at(std::string s);
    const Rules& at(std::string s) const;
    const Rules& operator[](std::string s) const;

    Status check(const Parfait::Dictionary& dict) const;

    std::string to_string() const;

    static std::string typeToString(Type t);

    static Type TypeFromString(std::string t);

  private:
    std::string help;
    std::vector<Rules> one_of;
    std::map<std::string, Rules> properties;
    std::set<std::string> required_keys;
    std::set<std::string> enums;
    Type required_type = unset;
    double minimum = std::numeric_limits<double>::lowest();
    double maximum = std::numeric_limits<double>::max();
    double minimum_length = 0;
    double maximum_length = std::numeric_limits<int>::max();
    bool allow_additional_properties = true;

    bool check_required_value = false;
    bool required_value_bool;
    double required_value_number;
    std::string required_value_string;

    Status isEnum(const Parfait::Dictionary& dict) const;
    Status checkAdditionalProperties(const Parfait::Dictionary& dict) const;
    Status hasAllRequiredKeys(const Parfait::Dictionary& dict) const;
    Status isRequiredType(const Parfait::Dictionary& dict) const;
    Status isRequiredValue(const Parfait::Dictionary& dict) const;
    Status isWithinNumericBounds(const Parfait::Dictionary& dict) const;
    Status isWithinArrayLengthBounds(const Parfait::Dictionary& dict) const;
    bool isNumber(Parfait::DictionaryEntry::TYPE t) const;
    bool isArray(Parfait::DictionaryEntry::TYPE t) const;
    Status isSame(Parfait::DictionaryEntry::TYPE t1, Type t2) const;
    Status checkChildren(const Parfait::Dictionary& dict) const;
    Status checkOneOf(const Parfait::Dictionary& dict) const;
};
}