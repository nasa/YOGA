
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

#include <limits>
#include "Extent.h"

namespace Parfait {
namespace ExtentBuilder {

    template <typename T, size_t N = 3>
    Extent<T, N> createEmptyBuildableExtent() {
        Extent<T, N> e;
        for (size_t i = 0; i < N; i++) {
            e.hi[i] = std::numeric_limits<T>::lowest();
            e.lo[i] = std::numeric_limits<T>::max();
        }
        return e;
    }

    template <typename T>
    Extent<T> createMaximumExtent() {
        T hi = std::numeric_limits<T>::max();
        T lo = std::numeric_limits<T>::lowest();
        return Extent<T>{{lo, lo, lo}, {hi, hi, hi}};
    }

    template <typename T, size_t N = 3>
    void add(Extent<T, N>& e, const Point<T, N>& p) {
        for (size_t i = 0; i < N; i++) {
            e.lo[i] = std::min(e.lo[i], p[i]);
            e.hi[i] = std::max(e.hi[i], p[i]);
        }
    }

    template <typename T, size_t N>
    void add(Extent<T, N>& e1, const Extent<T, N>& e2) {
        for (size_t i = 0; i < N; i++) {
            e1.lo[i] = std::min(e1.lo[i], e2.lo[i]);
            e1.hi[i] = std::max(e1.hi[i], e2.hi[i]);
        }
    }

    template <typename T, size_t N>
    Extent<T, N> getBoundingBox(const std::vector<Extent<T, N>>& boxes) {
        if (boxes.empty()) return Extent<T, N>();
        Extent<T, N> out{boxes.front()};
        for (const auto& ext : boxes) add(out, ext);
        return out;
    }

    template <typename T>
    Extent<T> intersection(Extent<T>& e1, const Extent<T>& e2) {
        if (not e1.intersects(e2)) return createEmptyBuildableExtent<T>();
        Extent<T> e;
        for (int i = 0; i < 3; i++) e.lo[i] = (e1.lo[i] < e2.lo[i]) ? (e2.lo[i]) : (e1.lo[i]);

        for (int i = 0; i < 3; i++) e.hi[i] = (e1.hi[i] > e2.hi[i]) ? (e2.hi[i]) : (e1.hi[i]);
        return e;
    }

    template <typename Container, size_t N = 3>
    auto build(const Container& points) {
        auto e = createEmptyBuildableExtent<double, N>();
        for (auto& p : points) add(e, p);
        return e;
    }

    template <typename GetPoint, size_t N = 3>
    Extent<double> build(const GetPoint& get_point, int num_points) {
        auto e = createEmptyBuildableExtent<double, N>();
        for (int i = 0; i < num_points; i++) {
            add(e, get_point(i));
        }
        return e;
    }

    template <typename MeshType>
    Extent<double> buildExtentForCellInMesh(const MeshType& mesh, int cell_id) {
        std::vector<int> cell;
        mesh.cell(cell_id, cell);
        auto e = createEmptyBuildableExtent<double>();
        for (int node_id : cell) {
            auto xyz = mesh.node(node_id);
            add(e, Point<double>(xyz[0], xyz[1], xyz[2]));
        }
        return e;
    }

    template <typename MeshType>
    Extent<double> buildExtentForPointsInMesh(const MeshType& mesh) {
        auto e = createEmptyBuildableExtent<double>();
        for (int node_id = 0; node_id < mesh.nodeCount(); node_id++) {
            auto xyz = mesh.node(node_id);
            add(e, Point<double>(xyz[0], xyz[1], xyz[2]));
        }
        return e;
    }

    template <typename MeshType>
    Extent<double> buildExtentForBoundaryFaceInMesh(const MeshType& mesh, int face_id) {
        Extent<double> e{{1e20, 1e20, 1e20}, {-1e20, -1e20, -1e20}};
        auto face = mesh.getNodesInBoundaryFace(face_id);
        for (int node_id : face) {
            auto xyz = mesh.getNode(node_id);
            add(e, Point<double>(xyz[0], xyz[1], xyz[2]));
        }
        return e;
    }
}
}