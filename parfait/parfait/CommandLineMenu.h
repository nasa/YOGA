#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "CommandLine.h"
#include <parfait/ToString.h>
#include <parfait/StringTools.h>
#include "Throw.h"

namespace Parfait {

class CommandLineMenu {
  public:
    typedef const std::string& String;
    typedef const std::vector<std::string>& Strings;
    CommandLineMenu() : silence_exceptions(false) {}
    CommandLineMenu(bool quiet) : silence_exceptions(quiet) {}
    void silence() { silence_exceptions = true; }

    void addFlag(Strings aliases, String help_string) {
        auto dashed_aliases = formatAliases(aliases);
        acceptable_flags.push_back(Flag(dashed_aliases, help_string));
    }
    void addHiddenFlag(Strings aliases, String help_string) {
        auto dashed_aliases = formatAliases(aliases);
        acceptable_flags.push_back(Flag(dashed_aliases, help_string, true));
    }

    void addParameter(Strings aliases, String help_string, bool required, String default_val = "") {
        auto dashed_aliases = formatAliases(aliases);
        acceptable_parameters.push_back(Parameter(dashed_aliases, help_string, required, default_val));
    }

    void addHiddenParameter(Strings aliases, String help_string, String default_val = "") {
        bool required = false;
        auto dashed_aliases = formatAliases(aliases);
        acceptable_parameters.push_back(Parameter(dashed_aliases, help_string, required, default_val, true));
    }

    bool parse(int argc, const char* argv[]) {
        auto args = CommandLine::getCommandLineArgs(argc, argv);
        args.erase(args.begin(), args.begin() + 1);
        return parse(args);
    }

    bool parse(String s) { return parse(Parfait::StringTools::split(s, " ")); }

    bool parse(Strings args) {
        for (int i = 0; i < int(args.size()); i++) {
            const auto& arg = args[i];
            if (isAcceptableFlag(arg))
                selectFlag(arg);
            else if (isAcceptableParameter(arg))
                selectParameter(args, i);
            else if (silence_exceptions)
                return false;
            else
                return throwUnrecognized(arg);
        }
        appendDefaults();
        if (not areRequiredParamsSet()) {
            if (silence_exceptions)
                return false;
            else
                return throwMissing();
        }
        return true;
    }

    bool hasValid(String s) const {
        if (has(s)) {
            return not get(s).empty();
        }
        return false;
    }

    bool has(Strings s) const { return has(s.front()); }

    bool has(String s) const {
        auto key = formatDashPrefix(s);
        for (auto& f : selected_flags)
            if (f == key) return true;
        for (auto& f : selected_parameters)
            if (f == key) return true;
        return false;
    }

    std::string get(Strings s) const { return get(s.front()); }

    std::string get(String s) const {
        auto key = formatDashPrefix(s);
        auto v = getAsVector(key);
        if (v.empty())
            return "";
        else
            return v.front();
    }

    int getInt(String s) const {
        auto v = get(s);
        return Parfait::StringTools::toInt(v);
    }

    double getDouble(Strings s) const { return getDouble(s.front()); }

    double getDouble(String s) const {
        auto v = get(s);
        return Parfait::StringTools::toDouble(v);
    }

    std::vector<std::string> getAsVector(Strings s) const { return getAsVector(s.front()); }

    std::vector<std::string> getAsVector(String s) const {
        auto key = formatDashPrefix(s);
        for (size_t i = 0; i < selected_parameters.size(); i++)
            if (key == selected_parameters[i]) return parameter_values[i];
        PARFAIT_THROW("Could not find parameter " + s +
                      "\nAvailable parameters are: " + Parfait::to_string(selected_parameters));
    }

    std::string to_string_header(int column_width) { return ""; }

    std::string to_string() {
        int column_width = 80;
        auto formatted_menu = to_string_header(column_width);

        for (auto& param : getVisibleParameters()) formatted_menu += param.to_string(column_width);
        for (auto& flag : getVisibleFlags()) formatted_menu += flag.to_string(column_width);
        return formatted_menu;
    }

    std::string to_string_include_hidden() {
        int column_width = 80;
        auto formatted_menu = to_string_header(column_width);
        for (auto& param : acceptable_parameters) formatted_menu += param.to_string(column_width);
        for (auto& flag : acceptable_flags) formatted_menu += flag.to_string(column_width);
        return formatted_menu;
    }

    static bool isArgument(String s) {
        if (s.empty()) return false;
        if (s.front() != '-') return true;
        if (s.size() > 1) {
            if (s[1] == '.' or std::isdigit(s[1])) return true;
        }
        return false;
    }

    class Option {
      public:
        Option(Strings aliases, String help_string, bool hidden)
            : help_string(help_string), aliases(aliases), hidden(hidden) {}

        std::string to_string(int column_width) {
            int alias_spaces = 2;
            int help_spaces = 8;

            auto alias_string = StringTools::join(aliases, " ");
            alias_string += " " + decorator() + " " + require();

            std::string s;
            s += indent(alias_string, column_width, alias_spaces);
            s += indent(help_string, column_width, help_spaces);
            s += "\n";

            return s;
        }

        const std::string help_string;
        const std::vector<std::string> aliases;
        const bool hidden;
        virtual ~Option() {}

      protected:
        virtual std::string decorator() = 0;
        virtual std::string require() = 0;

      private:
        std::string indent(const std::string& s, int column_width, int indent_size) {
            auto lines = StringTools::wrapLines(s, column_width - indent_size);
            std::string indent(indent_size, ' ');
            std::string formatted;
            for (auto line : lines) formatted += indent + line + "\n";
            return formatted;
        }
    };

    class Flag : public Option {
      public:
        Flag(Strings aliases, String help_string, bool hidden = false) : Option(aliases, help_string, hidden){};

      private:
        virtual std::string decorator() override { return ""; }
        virtual std::string require() override { return ""; }
    };

    class Parameter : public Option {
      public:
        Parameter(Strings aliases, String help_string, bool required, String default_val, bool hidden = false)
            : Option(aliases, help_string, hidden), required(required), default_val(default_val){};
        const bool required;
        const std::string default_val;

      private:
        virtual std::string decorator() override { return default_val.empty() ? "<value>" : "<" + default_val + ">"; }
        virtual std::string require() override { return required ? "[REQUIRED]" : ""; }
    };

    std::vector<double> getDoubles(std::string key) {
        std::vector<double> v;
        for(auto s: getAsVector(key))
            v.push_back(StringTools::toDouble(s));
        return v;
    }

    std::vector<int> getInts(std::string key) {
        std::set<int> vals;
        for(auto s: getAsVector(key))
            for(int n:StringTools::parseIntegerList(s))
                vals.insert(n);
        return std::vector<int>(vals.begin(),vals.end());
    }

  private:
    bool silence_exceptions;
    std::vector<Flag> acceptable_flags;
    std::vector<Parameter> acceptable_parameters;
    std::vector<std::string> selected_flags;
    std::vector<std::string> selected_parameters;
    std::vector<std::vector<std::string>> parameter_values;

    auto getFlags(bool hidden) {
        std::vector<Flag> flags;
        for (auto flag : acceptable_flags)
            if (hidden == flag.hidden) flags.push_back(flag);
        return flags;
    }
    std::vector<Flag> getHiddenFlags() { return getFlags(true); }
    std::vector<Flag> getVisibleFlags() { return getFlags(false); }
    auto getParameters(bool hidden) {
        std::vector<Parameter> params;
        for (auto param : acceptable_parameters)
            if (hidden == param.hidden and param.required) params.push_back(param);
        for (auto param : acceptable_parameters)
            if (hidden == param.hidden and not param.required) params.push_back(param);
        return params;
    }
    std::vector<Parameter> getHiddenParameters() { return getParameters(true); }
    std::vector<Parameter> getVisibleParameters() { return getParameters(false); }

    std::string formatDashPrefix(String s) const {
        int n = int(s.size());
        if (n == 1) return "-" + s;
        if (s.front() != '-') return "--" + s;
        return s;
    }

    std::vector<std::string> formatAliases(Strings aliases) const {
        std::vector<std::string> dashed_aliases = aliases;
        for (auto& alias : dashed_aliases) alias = formatDashPrefix(alias);
        return dashed_aliases;
    }

    bool isAcceptableFlag(String arg) {
        for (auto& flag : acceptable_flags)
            for (auto& alias : flag.aliases)
                if (arg == alias) return true;
        return false;
    }

    bool isAcceptableParameter(String arg) {
        for (auto& flag : acceptable_parameters)
            for (auto& alias : flag.aliases)
                if (arg == alias) return true;
        return false;
    }

    bool areRequiredParamsSet() {
        for (auto& param : acceptable_parameters)
            if (param.required and not has(param.aliases.front())) return false;
        return true;
    }

    void appendDefaults() {
        for (auto& param : acceptable_parameters) {
            if (not param.default_val.empty() and not has(param.aliases.front())) {
                for (auto& alias : param.aliases) {
                    selected_parameters.push_back(alias);
                    parameter_values.push_back({param.default_val});
                }
            }
        }
    }

    void selectFlag(String arg) {
        for (auto& flag : acceptable_flags) {
            for (auto& alias : flag.aliases) {
                if (arg == alias) {
                    for (auto& a : flag.aliases) selected_flags.push_back(a);
                    return;
                }
            }
        }
    }

    void selectParameter(Strings args, int& i) {
        auto parameter = args[i];
        std::vector<std::string> values;
        while ((i + 1) < int(args.size())) {
            if (not isArgument(args[i + 1])) break;
            values.push_back(args[i + 1]);
            i++;
        }
        for (auto& flag : acceptable_parameters) {
            for (auto& alias : flag.aliases) {
                if (parameter == alias) {
                    for (auto& a : flag.aliases) {
                        selected_parameters.push_back(a);
                        parameter_values.push_back(values);
                    }
                }
            }
        }
    }

    bool throwUnrecognized(String arg) {
        std::string msg = "Unrecognized argument: " + arg;
        msg += "\n" + to_string();
        msg += "\nKnown parameters are: \n";
        for (auto& p : acceptable_parameters) {
            for (auto& a : p.aliases) {
                msg += "  <" + a + ">\n";
            }
        }
        throw std::logic_error(msg);
        return false;
    }

    bool throwMissing() {
        std::string msg = "Missing required argument\n";
        msg += "\n" + to_string();
        throw std::logic_error(msg);
        return false;
    }
};
}