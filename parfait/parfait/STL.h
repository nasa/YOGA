
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

#include <stdint.h>
#include <string>
#include <vector>
#include "Adt3dExtent.h"
#include "ExtentBuilder.h"
#include "Facet.h"
#include "Point.h"

namespace Parfait {
namespace STL {

    std::vector<Facet> load(std::string filename);

    template <typename Container>
    void write(const Container& facets, std::string filename, std::string solid_name = "solid_name");
    template <typename Container>
    void writeBinaryFile(const Container& facets, std::string filename, std::string solidName = "solidName");

    std::vector<Facet> loadBinaryFile(std::string fileName);
    void writeAsciiFile(const std::vector<Facet>& facets, std::string fileName, std::string solidName = "solidName");
    void translateCenterToOrigin(std::vector<Facet>& facets);
    void translateCenterToOrigin(const Extent<double>& d, std::vector<Facet>& facets);
    void scaleToUnitLength(std::vector<Facet>& facets);
    void scaleToUnitLength(const Extent<double>& d, std::vector<Facet>& facet);
    void rescale(double scale, std::vector<Facet>& facet);
    double getLongestCartesianLength(const Extent<double>& d);
    Extent<double> findDomain(const std::vector<Facet>& facets);
    std::vector<Facet> convertToFacets(const std::vector<int>& node_ids,
                                       const std::vector<Parfait::Point<double>>& points);

    std::vector<std::vector<int>> buildNodeToNode(const std::vector<int>& triangle_node_ids, int node_count);
    std::vector<std::vector<int>> buildNodeToCell(const std::vector<int>& triangle_node_ids, int node_count);
    std::pair<std::vector<int>, int> labelShells(const std::vector<int>& triangle_nodes,
                                                 const std::vector<std::vector<int>>& node_to_cell);
    std::vector<std::vector<Parfait::Facet>> splitByLabel(const std::vector<int>& triangle_nodes,
                                                          const std::vector<Parfait::Point<double>>& points,
                                                          const std::vector<int>& triangle_labels,
                                                          int num_labels);
}
Parfait::Extent<double> findDomain(const std::vector<std::array<Parfait::Point<double>, 3>>& facets);
}

#include "STL.hpp"
