#pragma once
#include "MDVector.h"
#include "StructuredBlockFace.h"

namespace inf {
template <typename T>
class StructuredBlockPlane {
  public:
    StructuredBlockPlane() = default;
    template <typename GetValue>
    StructuredBlockPlane(const GetValue& get_value,
                         const std::array<int, 3>& dimensions,
                         BlockAxis axis,
                         int index)
        : original_block_dimensions(dimensions),
          slice_axis(axis),
          slice_index(index),
          plane(getPlaneDimensions()) {
        setSlice(get_value);
    }

    template <typename GetValue>
    void setSlice(const GetValue& get_value) {
        switch (slice_axis) {
            case I: {
                for (int j = 0; j < original_block_dimensions[1]; ++j) {
                    for (int k = 0; k < original_block_dimensions[2]; ++k) {
                        plane(j, k) = get_value(slice_index, j, k);
                    }
                }
                break;
            }
            case J: {
                for (int i = 0; i < original_block_dimensions[0]; ++i) {
                    for (int k = 0; k < original_block_dimensions[2]; ++k) {
                        plane(i, k) = get_value(i, slice_index, k);
                    }
                }
                break;
            }
            default: {
                for (int i = 0; i < original_block_dimensions[0]; ++i) {
                    for (int j = 0; j < original_block_dimensions[1]; ++j) {
                        plane(i, j) = get_value(i, j, slice_index);
                    }
                }
                break;
            }
        }
    }

    T& operator()(int i, int j, int k) {
        switch (slice_axis) {
            case I:
                assert(i == slice_index);
                return plane(j, k);
            case J:
                assert(j == slice_index);
                return plane(i, k);
            default:
                assert(k == slice_index);
                return plane(i, j);
        }
    }
    std::array<int, 3> original_block_dimensions;
    BlockAxis slice_axis;
    int slice_index;
    MDVector<T, 2> plane;

    std::array<int, 2> getPlaneDimensions() const {
        int d = 0;
        std::array<int, 2> plane_dimensions;
        for (auto axis : {I, J, K})
            if (axis != slice_axis) plane_dimensions[d++] = original_block_dimensions[axis];
        return plane_dimensions;
    }
};

}