
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
#include <algorithm>
#include <stack>
namespace Parfait {

template <int ndim>
void Adt<ndim>::reserve(size_t n) {
    element_tags.reserve(n);
    object_extents.reserve(n);
    left_child_ids.reserve(n);
    right_child_ids.reserve(n);
    hyper_boxes.reserve(n);
}

template <int ndim>
std::vector<int> Adt<ndim>::retrieve(const double* extent) const {
    std::vector<int> tags;
    if (element_tags.empty()) {
        return tags;
    }
    HyperBox query_region(extent);
    retrieve(0, tags, query_region);
    return tags;
}

template <int ndim>
void Adt<ndim>::retrieve(const double* extent, std::vector<int>& ids) const {
    ids.clear();
    if (element_tags.empty()) {
        return;
    }
    HyperBox query_region(extent);
    retrieve(0, ids, query_region);
}

template <int ndim>
void Adt<ndim>::store(int tag, const double* x) {
    HyperBox root, new_leaf;
    root.min.fill(0.0);
    root.max.fill(1.0);
    Box object;
    std::copy(x, x + ndim, object.begin());
    new_leaf.shrink(object);
    if (element_tags.empty()) {
        element_tags.push_back(tag);
        object_extents.push_back(object);
        left_child_ids.push_back(0);
        right_child_ids.push_back(0);
        hyper_boxes.push_back(new_leaf);
    } else {
        int id = 0;
        int current_depth = 0;
        store(id, tag, current_depth, root, object, new_leaf);
    }
}

template <int ndim>
void Adt<ndim>::HyperBox::split(int current_depth, int which_child) {
    int split_axis = current_depth % ndim;
    double midpoint = 0.5 * (max[split_axis] + min[split_axis]);
    if (which_child == LEFT)
        max[split_axis] = midpoint;
    else
        min[split_axis] = midpoint;
}

template <int ndim>
void Adt<ndim>::store(
    int id, int tag, int current_depth, HyperBox& current_span, const Box& object, const HyperBox& new_leaf) {
    hyper_boxes[id].expand(new_leaf);
    auto which_child = current_span.determineChild(current_depth, object);
    current_span.split(current_depth, which_child);

    if (which_child == LEFT) {
        if (left_child_ids[id] == 0) {
            left_child_ids[id] = element_tags.size();
            element_tags.push_back(tag);
            object_extents.push_back(object);
            left_child_ids.push_back(0);
            right_child_ids.push_back(0);
            hyper_boxes.push_back(new_leaf);
        } else {
            store(left_child_ids[id], tag, current_depth + 1, current_span, object, new_leaf);
        }
    } else {
        if (right_child_ids[id] == 0) {
            right_child_ids[id] = element_tags.size();
            element_tags.push_back(tag);
            object_extents.push_back(object);
            left_child_ids.push_back(0);
            right_child_ids.push_back(0);
            hyper_boxes.push_back(new_leaf);
        } else {
            store(right_child_ids[id], tag, current_depth + 1, current_span, object, new_leaf);
        }
    }
}

template <int ndim>
void Adt<ndim>::retrieve(int id, std::vector<int>& tags, const HyperBox& query_region) const {
    if (!hyper_boxes[id].contains(query_region)) {
        return;
    }
    if (query_region.contains(object_extents[id])) {
        tags.push_back(element_tags[id]);
    }
    int left_child = left_child_ids[id];
    int right_child = right_child_ids[id];
    if (left_child > 0) {
        retrieve(left_child, tags, query_region);
    }
    if (right_child > 0) {
        retrieve(right_child, tags, query_region);
    }
}

template <int ndim>
Adt<ndim>::HyperBox::HyperBox(const double* extent) {
    if (ndim == 2) {
        min[0] = extent[0];
        min[1] = extent[1];
        max[0] = extent[2];
        max[1] = extent[3];
    } else if (ndim == 3) {
        min[0] = extent[0];
        min[1] = extent[1];
        min[2] = extent[2];
        max[0] = extent[3];
        max[1] = extent[4];
        max[2] = extent[5];
    } else if (ndim == 4)  // 2d extent box
    {
        double dx, dy;
        dx = extent[2] - extent[0];
        dy = extent[3] - extent[1];
        min[0] = 0.0;
        min[1] = 0.0;
        min[2] = extent[2] - dx;
        min[3] = extent[3] - dy;
        max[0] = extent[0] + dx;
        max[1] = extent[1] + dy;
        max[2] = 1.0;
        max[3] = 1.0;
    } else if (ndim == 6)  // 3d extent box
    {
        double dx, dy, dz;
        dx = extent[3] - extent[0];
        dy = extent[4] - extent[1];
        dz = extent[5] - extent[2];
        min[0] = 0.0;
        min[1] = 0.0;
        min[2] = 0.0;
        min[3] = extent[3] - dx;
        min[4] = extent[4] - dy;
        min[5] = extent[5] - dz;
        max[0] = extent[0] + dx;
        max[1] = extent[1] + dy;
        max[2] = extent[2] + dz;
        max[3] = 1.0;
        max[4] = 1.0;
        max[5] = 1.0;
    } else {
        throw std::logic_error(
            "ADT ERROR: Only 2d and 3d extent boxes can be turned"
            " into hyper rectangles\n");
    }
}

template <int ndim>
typename Adt<ndim>::ChildType Adt<ndim>::HyperBox::determineChild(int depth, const Box& x) {
    int split_axis = depth % ndim;
    double midpoint = 0.5 * (max[split_axis] + min[split_axis]);
    if (x[split_axis] < midpoint)
        return LEFT;
    else
        return RIGHT;
}

template <int ndim>
int Adt<ndim>::nextIdInPostOrderTraversal(int starting_id) const {
    int left = left_child_ids[starting_id];
    int right = right_child_ids[starting_id];
    if (left > 0) {
        return nextIdInPostOrderTraversal(left);
    } else if (right > 0) {
        return nextIdInPostOrderTraversal(right);
    } else {
        return starting_id;
    }
}

template <int ndim>
void Adt<ndim>::expandParent(int parent_id, int child_id) {
    auto& parent_xmin = hyper_boxes[parent_id].min;
    auto& parent_xmax = hyper_boxes[parent_id].max;
    auto& child_xmin = hyper_boxes[child_id].min;
    auto& child_xmax = hyper_boxes[child_id].max;
    for (int i = 0; i < ndim; i++) {
        parent_xmin[i] = std::min(parent_xmin[i], child_xmin[i]);
        parent_xmax[i] = std::max(parent_xmax[i], child_xmax[i]);
    }
}

template <int ndim>
std::vector<int> Adt<ndim>::buildParentList() const {
    std::vector<int> parent(element_tags.size(), -1);
    for (size_t i = 0; i < element_tags.size(); i++) {
        int left = left_child_ids[i];
        int right = right_child_ids[i];
        if (left > 0) parent[left] = i;
        if (right > 0) parent[right] = i;
    }
    return parent;
}

template <int ndim>
void Adt<ndim>::tighten() {
    auto parent_ids = buildParentList();
    shrinkElementsToFitTheirObjects();
    int next_id = nextIdInPostOrderTraversal(0);
    while (0 != next_id) {
        int parent_id = parent_ids[next_id];
        expandParent(parent_id, next_id);
        if (left_child_ids[parent_id] == next_id) {
            int sibling = right_child_ids[parent_id];
            if (sibling > 0) {
                next_id = nextIdInPostOrderTraversal(sibling);
            } else {
                next_id = parent_id;
            }
        } else {
            next_id = parent_id;
        }
    }
    is_tightened = true;
}

template <int ndim>
void Adt<ndim>::HyperBox::shrink(const Box& e) {
    std::transform(e.begin(), e.end(), min.begin(), [](double x) { return std::max(x, 0.); });
    std::transform(e.begin(), e.end(), max.begin(), [](double x) { return std::min(x, 1.); });
}

template <int ndim>
void Adt<ndim>::HyperBox::expand(const Parfait::Adt<ndim>::HyperBox& other) {
    for (int i = 0; i < ndim; i++) {
        min[i] = std::min(min[i], other.min[i]);
        max[i] = std::max(max[i], other.max[i]);
    }
}

template <int ndim>
void Adt<ndim>::shrinkElementsToFitTheirObjects() {
    hyper_boxes.resize(element_tags.size());
    for (size_t id = 0; id < element_tags.size(); id++) {
        auto& element_span = hyper_boxes[id];
        auto& e = object_extents[id];
        element_span.shrink(e);
    }
}

template <int ndim>
bool Adt<ndim>::HyperBox::contains(const Box& object) const {
    bool does_contain = true;
    for (int i = 0; i < ndim; i++)
        if (max[i] < object[i] - ADT_TOL || min[i] > object[i] + ADT_TOL) does_contain = false;
    return does_contain;
}
template <int ndim>
bool Adt<ndim>::HyperBox::contains(const HyperBox& other) const {
    bool does_contain = true;
    for (int i = 0; i < ndim; i++)
        if (other.max[i] < min[i] - ADT_TOL || other.min[i] > max[i] + ADT_TOL) does_contain = false;
    return does_contain;
}

}
