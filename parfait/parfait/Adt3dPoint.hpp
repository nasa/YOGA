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
#include <stdexcept>

inline Parfait::Adt3DPoint::Adt3DPoint(const Parfait::Extent<double>& domain) : unitTransformer(domain) {}

inline std::vector<int> Parfait::Adt3DPoint::retrieve(const Parfait::Extent<double>& domain) const {
    Parfait::Extent<double> search(unitTransformer.ToUnitSpace(domain.lo), unitTransformer.ToUnitSpace(domain.hi));
    return adt.retrieve(&search.lo[0]);
}

inline bool Parfait::Adt3DPoint::isPointInAdt(const Point<double>& p, int& tag, double tol) const {
    auto box = Extent<double>::extentAroundPoint(p, tol);
    auto inside = this->retrieve(box);

    if (0 == inside.size()) {
        return false;
    } else if (1 == inside.size()) {
        tag = inside[0];
        return true;
    } else {
        throw std::domain_error(
            "Error: Nodes too close trying to create unique mapping of "
            "There are " +
            std::to_string(inside.size()) + " nodes inside this box");
    }
}

inline void Parfait::Adt3DPoint::store(int tag, const Parfait::Point<double>& point) {
    auto storePoint = unitTransformer.ToUnitSpace(point);
    for (int i = 0; i < 3; i++) {
        if (storePoint[i] < 0 or storePoint[i] > 1.0) {
            throw std::logic_error("Cannot store points in ADT which are outside the ADT rootDomain.");
        }
    }
    adt.store(tag, storePoint.data());
}