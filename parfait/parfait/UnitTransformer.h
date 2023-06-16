
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
#include "Point.h"
#include "Extent.h"

namespace Parfait {
template <typename T>
class UnitTransformer {
  public:
    UnitTransformer(const Extent<T>& extent_i) : extent(extent_i) { GetScale(); }

    inline Extent<T> getDomain() const { return extent; }

    inline T GetScale() {
        auto dx = extent.hi[0] - extent.lo[0];
        auto dy = extent.hi[1] - extent.lo[1];
        auto dz = extent.hi[2] - extent.lo[2];
        T ddum = (dx > dy) ? (dx) : (dy);
        scale = (ddum > dz) ? (ddum) : (dz);
        oneOverScale = 1.0 / scale;
        return scale;
    }

    inline Point<T> ToUnitSpace(const Point<T>& physicalPoint) const {
        Point<T> temp(physicalPoint);
        temp[0] -= extent.lo[0];
        temp[1] -= extent.lo[1];
        temp[2] -= extent.lo[2];

        temp[0] *= oneOverScale;
        temp[1] *= oneOverScale;
        temp[2] *= oneOverScale;

        return temp;
    }

    inline Point<T> ToPhysicalSpace(const Point<T>& unitPoint) const {
        Point<T> temp(unitPoint);
        temp[0] *= scale;
        temp[1] *= scale;
        temp[2] *= scale;

        temp[0] += extent.lo[0];
        temp[1] += extent.lo[1];
        temp[2] += extent.lo[2];

        return temp;
    }

  private:
    Extent<T> extent;
    T scale;
    T oneOverScale;
};
}