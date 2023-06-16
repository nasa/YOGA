#pragma once

#include <string>
#include "Throw.h"

namespace Parfait {
class DictionaryEntry {
  public:
    enum TYPE {
        Object,
        String,
        Integer,
        Double,
        Boolean,
        Null,
        BoolArray,
        IntArray,
        DoubleArray,
        StringArray,
        ObjectArray
    };
    typedef const std::string& Key;
    virtual TYPE type() const = 0;
    std::string typeString() const {
        switch (type()) {
            case Object:
                return "object";
            case String:
                return "string";
            case Integer:
                return "integer";
            case Double:
                return "double";
            case Boolean:
                return "boolean";
            case Null:
                return "null";
            case BoolArray:
                return "boolarray";
            case IntArray:
                return "intarray";
            case DoubleArray:
                return "doublearray";
            case StringArray:
                return "stringarray";
            case ObjectArray:
                return "objectarray";
            default:
                return "Error: unknown type";
        }
    }
    inline virtual std::string asString() const {
        throwCantConvertToType(String);
        return "";
    }
    inline virtual int asInt() const {
        throwCantConvertToType(Integer);
        return 0;
    }
    inline virtual double asDouble() const {
        throwCantConvertToType(Double);
        return 0;
    }
    inline virtual bool asBool() const {
        throwCantConvertToType(Boolean);
        return false;
    }
    virtual ~DictionaryEntry() = default;

  protected:
    inline void throwCantConvertToType(TYPE t) const {
        PARFAIT_THROW("Can't get " + typeString(this->type()) + " as " + typeString(t));
    }

  private:
    inline std::string typeString(TYPE t) const {
        switch (t) {
            case Object:
                return "Object";
            case String:
                return "String";
            case Integer:
                return "Integer";
            case Double:
                return "Double";
            case Boolean:
                return "Boolean";
            case Null:
                return "Null";
            case BoolArray:
                return "BoolArray";
            case IntArray:
                return "IntArray";
            case DoubleArray:
                return "DoubleArray";
            case StringArray:
                return "StringArray";
            case ObjectArray:
                return "ObjectArray";
            default:
                return "";
        }
    }
};
}