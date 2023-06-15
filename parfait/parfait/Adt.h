#pragma once

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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>
#include "UnitTransformer.h"

namespace Parfait {

#define ADT_TOL 1.0e-12
//------------------------------------------------------------------
// Explanation of how the dimension of the tree is
// chosen:
// ----- All objects have to be stored as points
// -- A 2d point can obviously be stored as a 2d point
// -- A 3d point can be stored as a 3d point
// -- A 2d extent has 4 components (xmin,ymin)(xmax,ymax),
// 			so it can be viewed as a "point" in 4d space
// -- A 3d extent has 6 components (xmin,ymin,zmin)(xmax,ymax,zmax),
// 			so it can be viewed as a "point" in 6d space
//------------------------------------------------------------------
//  For further explanation, see
//  	An Alternating Digial Tree (ADT) Algorithm for 3D Geometric
//  	Searching and Intersection Problems.
//  	Jaime Peraire, Javier Bonet
//  	International Journal for Numerical Methods In Eng, vol 31,
//  	1-17, (1991)
//------------------------------------------------------------------

template <int ndim>
class Adt {
  public:
    void store(int tag, const double* x);
    std::vector<int> retrieve(const double* extent) const;
    void retrieve(const double* extent, std::vector<int>& ids) const;
    void tighten();
    void reserve(size_t n);
    size_t size() const { return element_tags.size(); }

  private:
    typedef std::array<double, ndim> Box;
    enum ChildType { LEFT, RIGHT };
    class HyperBox {
      public:
        Box min;
        Box max;

        HyperBox() = default;
        HyperBox(const Box& min, const Box& max) : min(min), max(max) {}
        explicit HyperBox(const double* extent);
        void split(int depth, int which_child);
        void shrink(const Box& e);
        void expand(const HyperBox& other);
        bool contains(const HyperBox& other) const;
        bool contains(const Box& object) const;
        ChildType determineChild(int depth, const Box& x);
    };
    bool is_tightened = false;
    std::vector<int> element_tags;
    std::vector<Box> object_extents;
    std::vector<int> left_child_ids;
    std::vector<int> right_child_ids;
    std::vector<HyperBox> hyper_boxes;

    void store(int id, int tag, int current_depth, HyperBox& current_span, const Box& object, const HyperBox& new_leaf);
    void retrieve(int id, std::vector<int>& tags, const HyperBox& query_region) const;
    int nextIdInPostOrderTraversal(int starting_id) const;
    std::vector<int> buildParentList() const;
    void shrinkElementsToFitTheirObjects();
    void expandParent(int parent_id, int child_id);
};
}
#include "Adt.hpp"