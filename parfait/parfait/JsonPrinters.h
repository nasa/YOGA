#pragma once

#include "DictionaryEntry.h"
#include <iomanip>
#include <cmath>

namespace Parfait {
namespace Json {
    class PrintingVisitor {
      public:
        explicit PrintingVisitor(int indent_size) : intent(indent_size) {}
        virtual ~PrintingVisitor() = default;
        template <typename Json>
        inline std::string dump(const Json& json) {
            return dump(json, 0);
        }

      private:
        int intent;
        virtual std::string formatKey(std::string s) = 0;
        virtual std::string formatValue(int n) = 0;
        virtual std::string formatValue(double n) = 0;
        virtual std::string formatValue(bool b) = 0;
        virtual std::string formatValue(std::string s) = 0;
        virtual std::string formatIndent(int indent) = 0;
        virtual std::string formatNull() { return "null"; }

        std::string newLine(int level) { return formatIndent(level * intent); }

        template <typename Json>
        inline std::string dump(const Json& json, int level) {
            if (json.isMappedObject())
                return dumpMapped(json, level);
            else if (json.isArrayObject())
                return dumpArray(json, level);
            else if (json.isValueObject())
                return dumpValue(json);
            return "{}";
        }

        template <typename Json>
        inline std::string dumpMapped(const Json& json, int level) {
            std::string s = "{";
            level++;
            for (auto& key : json.keys()) {
                s += newLine(level);
                s += formatKey(key);
                s += dump(json.at(key), level);
                s += ",";
            }
            level--;
            if (json.size() > 0) {
                s.pop_back();
                s += newLine(level);
            }
            s += "}";
            return s;
        }
        template <typename Json>
        inline std::string dumpArray(const Json& json, int level) {
            std::string s = "[";
            level++;
            for (int i = 0; i < json.size(); i++) {
                s += newLine(level);
                s += dump(json.at(i), level);
                s += ",";
            }
            level--;
            if (json.size() > 0) {
                s.pop_back();
                s += newLine(level);
            }
            s += "]";
            return s;
        }
        template <typename Json>
        inline std::string dumpValue(const Json& json) {
            if (DictionaryEntry::Integer == json.type())
                return formatValue(json.asInt());
            else if (DictionaryEntry::Double == json.type())
                return formatValue(json.asDouble());
            else if (DictionaryEntry::Boolean == json.type())
                return formatValue(json.asBool());
            else if (DictionaryEntry::Null == json.type())
                return formatNull();
            else
                return formatValue(json.asString());
        }
    };

    inline bool doubleIsInt(double x) {
        double intpart;
        return std::modf(x, &intpart) == 0.0;
    }
    inline std::string toString(double x, int precision) {
        // If anyone knows the "C++ way" to prevent 1.0e200 becoming -> 1e200, please help...
        std::ostringstream stream;
        stream << std::setprecision(precision) << x;
        return stream.str();
    }

    class CompactPrinter : public PrintingVisitor {
      public:
        CompactPrinter() : PrintingVisitor(0) {}

      private:
        inline std::string formatKey(std::string s) override { return formatValue(s) + ":"; }
        inline std::string formatValue(int n) override { return toString(n, 16); }
        inline std::string formatValue(double n) override { return toString(n, 16); }
        inline std::string formatValue(bool b) override { return b ? "true" : "false"; }
        inline std::string formatValue(std::string s) override { return "\"" + s + "\""; }
        inline std::string formatIndent(int indent) override { return ""; }
    };

    class PrettyPrinter : public PrintingVisitor {
      public:
        explicit PrettyPrinter(int indent_size) : PrintingVisitor(indent_size) {}

      private:
        inline std::string formatKey(std::string s) override { return formatValue(s) + ": "; }
        inline std::string formatValue(int n) override { return toString(n, 16); }
        inline std::string formatValue(double n) override { return toString(n, 16); }
        inline std::string formatValue(bool b) override { return b ? "true" : "false"; }
        inline std::string formatValue(std::string s) override { return "\"" + s + "\""; }
        inline std::string formatIndent(int indent) override {
            std::string out = "\n";
            for (int i = 0; i < indent; ++i) out += " ";
            return out;
        }
    };

}
}
