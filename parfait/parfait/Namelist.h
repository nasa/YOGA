#pragma once
#include <parfait/NamelistParser.h>

namespace Parfait {

class Namelist {
  public:
    typedef const std::string& String;
    typedef std::vector<std::string> Strings;
    Namelist(String namelist_content) : dict(NamelistParser::parse(namelist_content)) {
        printf("Namelist:\n%s\n", dict.dump(1).c_str());
    }

    Strings namelists() {
        Strings names;
        for (int i = 0; i < dict.size(); i++) names.push_back(dict[i].keys().front());
        return names;
    }

    bool has(String namelist_name) {
        auto names = namelists();
        return std::count(names.begin(), names.end(), namelist_name) == 1;
    }

    bool has(String namelist_name, String key) {
        if (has(namelist_name)) {
            auto& entry = get(namelist_name);
            return entry.has(key);
        }
        return false;
    }

    Dictionary get(String name, String key) {
        if (not has(name, key)) return Dictionary();
        return get(name)[key];
    }

    void set(String name, String key, String value) { setGeneric(name, key, value); }

    void set(String name, String key, const char* value) { set(name, key, std::string(value)); }

    void set(String name, String key, bool value) { setGeneric(name, key, value); }

    void set(String name, String key, double value) { setGeneric(name, key, value); }

    void set(String name, String key, int value) { setGeneric(name, key, value); }

    void set(String name, String key, std::initializer_list<int> values) { set(name, key, std::vector<int>(values)); }

    void set(String name, String key, std::initializer_list<double> values) {
        set(name, key, std::vector<double>(values));
    }

    template <typename T>
    void set(String name, String key, const std::vector<T>& value) {
        setGeneric(name, key, value);
    }

    void patch(String namelist_patch) {
        Namelist patch(namelist_patch);
        for (auto name : patch.namelists())
            for (auto& key : patch.get(name).keys()) set(name, key, patch.get(name, key));
    }

    std::string to_string() {
        std::string indent = "    ";
        std::string s;
        for (auto name : namelists()) {
            s.append("&" + name + "\n");
            auto& entry = get(name);
            for (auto key : entry.keys()) {
                s.append(indent + key + " = ");
                appendValue(s, entry[key]);
                s.append("\n");
            }
            s.append("/\n\n");
        }
        return s;
    }

  private:
    Dictionary dict;

    Dictionary& get(String namelist_name) {
        if (not has(namelist_name)) throw std::logic_error("Namelist not found: " + namelist_name);
        for (int i = 0; i < dict.size(); i++)
            if (namelist_name == dict[i].keys().front()) return dict[i][namelist_name];
        return dict[0];
    }

    void add(String name, String key, const Dictionary& value) {
        if (not has(name)) {
            int n = dict.size();
            Dictionary d;
            d[name][key] = value;
            dict[n] = d;
        }
    }

    void set(String name, String key, const Dictionary& value) {
        if (not has(name)) {
            printf("Adding: %s: %s: %s\n", name.c_str(), key.c_str(), value.asString().c_str());
            add(name, key, value);
        } else {
            auto& entry = get(name);
            printf("Already has: %s\n", name.c_str());
            printf("   setting %s = ", key.c_str());
            printf("%s\n", value.dump().c_str());
            entry[key] = value;
        }
    }

    template <typename T>
    void setGeneric(String name, String key, T value) {
        Dictionary d;
        d = value;
        set(name, key, d);
    }

    void appendValue(std::string& s, const Dictionary& value) {
        if (Dictionary::TYPE::BoolArray == value.type()) {
            for (bool x : value.asBools()) {
                s.append(x ? ".true." : ".false.");
                s.append(", ");
            }
            s.resize(s.size() - 2);
        } else if (Dictionary::TYPE::IntArray == value.type()) {
            for (auto x : value.asInts()) s.append(std::to_string(x) + ", ");
            s.resize(s.size() - 2);
        } else if (Dictionary::TYPE::DoubleArray == value.type()) {
            for (auto x : value.asDoubles()) s.append(std::to_string(x) + ", ");
            s.resize(s.size() - 2);
        } else if (Dictionary::TYPE::StringArray == value.type()) {
            for (auto x : value.asStrings()) s.append("\"" + x + "\", ");
            s.resize(s.size() - 2);
        } else if (Dictionary::TYPE::Boolean == value.type()) {
            s.append("." + value.asString() + ".");
        } else if (Dictionary::TYPE::String == value.type()) {
            s.append("\"" + value.asString() + "\"");
        } else {
            s.append(value.asString());
        }
    }
};
}