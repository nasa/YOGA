
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

#include "CartBlock.h"
#include "MarchingCubes.h"
#include "STL.h"
#include <functional>
namespace Parfait {

namespace STLFactory {

    inline std::vector<Facet> generateSurface(std::function<double(const Parfait::Point<double>& p)> field,
                                              Parfait::Extent<double> e = {{0, 0, 0}, {1, 1, 1}},
                                              int nx = 25,
                                              int ny = 25,
                                              int nz = 25) {
        auto mesh = Parfait::CartBlock(e, nx, ny, nz);
        std::vector<Parfait::Facet> facets;
        for (int c = 0; c < mesh.numberOfCells(); c++) {
            auto e = mesh.createExtentFromCell(c);
            std::array<double, 8> scalar;
            auto corners = e.corners();
            for (int i = 0; i < 8; i++) {
                scalar[i] = field(corners[i]);
            }
            auto new_facets = Parfait::MarchingCubes::marchingCubes(e, scalar.data());
            facets.insert(facets.end(), new_facets.begin(), new_facets.end());
        }
        return facets;
    }

    inline std::vector<Facet> generateSphere(
        Parfait::Point<double> centroid, double radius = 0.8, int nx = 25, int ny = 25, int nz = 25) {
        auto signedDistance = [=](const Parfait::Point<double>& p) {
            auto mag = (centroid - p).magnitude();
            return radius - mag;
        };

        Parfait::Point<double> r{radius, radius, radius};
        auto lo = centroid - r * 2;
        auto hi = centroid + r * 2;
        return generateSurface(signedDistance, {lo, hi}, nx, ny, nz);
    }

    inline std::vector<Facet> getUnitCube() {
        std::vector<Facet> facets;

        Facet bottom1 = {{0, 0, 0}, {1, 1, 0}, {1, 0, 0}};
        facets.push_back(bottom1);

        Facet bottom2 = {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}};
        facets.push_back(bottom2);

        Facet front1 = {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}};
        facets.push_back(front1);

        Facet front2 = {{0, 0, 0}, {1, 0, 1}, {0, 0, 1}};
        facets.push_back(front2);

        Facet right1 = {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}};
        facets.push_back(right1);

        Facet right2 = {{1, 0, 0}, {1, 1, 1}, {1, 0, 1}};
        facets.push_back(right2);

        Facet back1 = {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}};
        facets.push_back(back1);

        Facet back2 = {{0, 1, 0}, {1, 1, 1}, {1, 1, 0}};
        facets.push_back(back2);

        Facet left1 = {{0, 0, 0}, {0, 1, 1}, {0, 1, 0}};
        facets.push_back(left1);

        Facet left2 = {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}};
        facets.push_back(left2);

        Facet top1 = {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}};
        facets.push_back(top1);

        Facet top2 = {{0, 0, 1}, {1, 1, 1}, {0, 1, 1}};
        facets.push_back(top2);

        return facets;
    }
}
}
