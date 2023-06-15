
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
#include <stdexcept>
#include <vector>

namespace Parfait {

class CRS {
  public:
    template <typename Vector>
    static std::vector<std::vector<int>> buildGraphFromCRS(const Vector& ia, const Vector& ja) {
        std::vector<std::vector<int>> graph(int(ia.size()) - 1);
        for (unsigned long row = 0; row < graph.size(); row++) {
            auto length = ia[row + 1] - ia[row];
            graph[row].reserve(length);
            for (auto index = ia[row]; index < ia[row + 1]; index++) {
                auto col = ja[index];
                graph[row].push_back(col);
            }
        }
        return graph;
    }
    template <typename DoOwn>
    inline static CRS buildCRSFromGraphAugmentWithOwnedDiagonals(const std::vector<std::vector<int>>& graph,
                                                                 DoOwn doOwn) {
        CRS crs;
        crs.ia.resize(graph.size() + 1);
        crs.iau.resize(graph.size(), -1);
        crs.ia[0] = 0;
        for (int i = 0; i < int(graph.size()); i++) {
            crs.ia[i + 1] = crs.ia[i] + int(graph[i].size());
            bool found_diagonal = false;
            for (int j = 0; j < int(graph[i].size()); j++) {
                auto id = graph[i][j];
                crs.ja.push_back(id);
                if (id == i) {
                    crs.iau[i] = crs.ia[i] + j;
                    found_diagonal = true;
                }
            }

            if (not found_diagonal and doOwn(i)) {
                auto last_index = crs.ia[i + 1];
                crs.ia[i + 1]++;
                crs.iau[i] = last_index;
                crs.ja.push_back(i);
            }
        }
        return crs;
    }
    inline static CRS buildCRSFromGraph(const std::vector<std::vector<int>>& graph) {
        CRS crs;
        crs.ia.resize(graph.size() + 1);
        crs.iau.resize(graph.size(), -1);
        crs.ia[0] = 0;
        for (int i = 0; i < int(graph.size()); i++) {
            crs.ia[i + 1] = crs.ia[i] + int(graph[i].size());
            for (int j = 0; j < int(graph[i].size()); j++) {
                auto id = graph[i][j];
                crs.ja.push_back(id);
                if (id == i) {
                    crs.iau[i] = crs.ia[i] + j;
                }
            }
        }
        return crs;
    }

    inline int getLocation(int i, int j) {
        if (i == j) {
            return iau[i];
        }
        for (auto search = ia[i]; search < ia[i + 1]; search++) {
            if (ja[search] == j) return search;
        }

        throw std::logic_error("Could not find location of item " + std::to_string(i) + " " + std::to_string(j));
    }

    std::vector<int> ia;
    std::vector<int> ja;
    std::vector<int> iau;
};
}
