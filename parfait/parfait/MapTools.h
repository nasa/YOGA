#pragma once
#include <map>
#include <set>
#include <vector>

namespace Parfait {

namespace MapTools {
    template <typename Key, typename Value>
    std::set<Key> keys(const std::map<Key, Value>& map) {
        std::set<Key> _keys;
        for (auto& pair : map) {
            _keys.insert(pair.first);
        }
        return _keys;
    }

    template <typename Value>
    std::map<Value, int> invert(const std::vector<Value>& input) {
        std::map<Value, int> g2l;

        for (int l = 0; l < int(input.size()); l++) {
            g2l[input[l]] = l;
        }

        return g2l;
    }

    template <typename Key, typename Value>
    std::map<Value, Key> invert(const std::map<Key, Value>& input) {
        std::map<Value, Key> out;

        for (auto& pair : input) {
            out[pair.second] = pair.first;
        }
        return out;
    }
}
}
