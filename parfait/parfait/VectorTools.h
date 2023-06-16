
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>

namespace Parfait {
namespace VectorTools {

    template <class Container, class T>
    bool isIn(const Container& container, T t) {
        for (auto u : container) {
            if (t == u) return true;
        }
        return false;
    }

    template <class T>
    bool oneOf(T t, const std::vector<T>& container) {
        return isIn(container, t);
    }
    inline bool oneOf(const char* t, const std::vector<std::string>& container) {
        return oneOf(std::string(t), container);
    }

    template <typename T>
    void insertUnique(std::vector<T>& vec, T n) {
        for (auto it = vec.begin(); it < vec.end(); it++) {
            if (n == *it)  // don't insert duplicate
                return;
            if (n < *it)  // insert in front of next biggest element
            {
                vec.insert(it, n);
                return;
            }
        }
        if (0 == (int)vec.size())  // if empty, push back
            vec.push_back(n);
        else if (vec.back() < n)  // if all elements smaller, push back
            vec.push_back(n);
    }

    template <typename T>
    void append(std::vector<T>& v1, const std::vector<T>& v2) {
        v1.reserve(v1.size() + v2.size());
        v1.insert(v1.end(), v2.begin(), v2.end());
    }

    template <typename T>
    std::vector<double> to_double(const std::vector<T>& vec) {
        std::vector<double> out(vec.size());
        for (unsigned long i = 0; i < out.size(); i++) {
            out[i] = double(vec[i]);
        }
        return out;
    }

    template <typename T>
    std::vector<long> to_long(const std::vector<T>& vec) {
        std::vector<long> out(vec.size());
        for (unsigned long i = 0; i < out.size(); i++) {
            out[i] = long(vec[i]);
        }
        return out;
    }

    template <typename T>
    std::vector<T> toVector(const std::set<T>& s) {
        std::vector<T> v(s.begin(), s.end());
        return v;
    }

    template <typename T>
    T min(const std::vector<T>& v) {
        return *std::min_element(v.begin(), v.end());
    }

    template <typename T>
    T max(const std::vector<T>& v) {
        return *std::max_element(v.begin(), v.end());
    }
    template <typename T>
    std::vector<T> range(T start, T end) {
        std::vector<T> v;
        for (T t = start; t < end; t++) {
            v.push_back(t);
        }
        return v;
    }
}
}