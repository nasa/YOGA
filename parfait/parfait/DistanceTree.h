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
#include <parfait/Extent.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/Facet.h>
#include <parfait/Point.h>
#include <limits>
#include <queue>
#include <stack>
#include <vector>
#include "LagrangeTriangle.h"
#include "OctreeStorage.h"
#include "EdgeSegmentProjection.h"

namespace Parfait {

class GeometrySegment {
  public:
    virtual Parfait::Extent<double> getExtent() const = 0;
    virtual Parfait::Point<double> getClosestPoint(const Parfait::Point<double>& p) const = 0;
    virtual bool intersects(const Parfait::Extent<double>& e) const = 0;

  protected:
    virtual ~GeometrySegment() = default;
};

class LineSegment : public GeometrySegment {
  public:
    inline LineSegment() : a(0, 0, 0), b(1, 1, 1) {}
    inline LineSegment(Parfait::Point<double> a_in, Parfait::Point<double> b_in) : a(a_in), b(b_in) {}
    inline Parfait::Extent<double> getExtent() const override {
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        Parfait::ExtentBuilder::add(e, a);
        Parfait::ExtentBuilder::add(e, b);
        return e;
    }
    inline Parfait::Point<double> getClosestPoint(const Parfait::Point<double>& p) const override {
        return Parfait::closestPointOnEdge(a, b, p);
    }
    inline bool intersects(const Parfait::Extent<double>& e) const override { return e.intersectsSegment(a, b); }
    ~LineSegment() override = default;

  public:
    Parfait::Point<double> a;
    Parfait::Point<double> b;
};

class FacetSegment : public GeometrySegment, public Parfait::Facet {
  public:
    using Facet::Facet;
    using Facet::operator=;
    FacetSegment() = default;
    FacetSegment(const Facet& f);
    Parfait::Extent<double> getExtent() const override;
    Parfait::Point<double> getClosestPoint(const Parfait::Point<double>& p) const override;
    bool intersects(const Parfait::Extent<double>& e) const override;
};

class TriP2Segment : public GeometrySegment, public Parfait::LagrangeTriangle::P2 {
  public:
    using Parfait::LagrangeTriangle::P2::P2;
    using Parfait::LagrangeTriangle::P2::operator=;
    TriP2Segment(const Parfait::LagrangeTriangle::P2& tri);
    Parfait::Extent<double> getExtent() const override;
    Parfait::Point<double> getClosestPoint(const Parfait::Point<double>& p) const override;
    bool intersects(const Parfait::Extent<double>& e) const override;
};

class DistanceTree : public OctreeStorage<GeometrySegment> {
  public:
    typedef std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<>>
        PriorityQueue;
    class CurrentState {
      public:
        CurrentState(Parfait::Point<double> q);
        bool isPotentiallyCloser(Parfait::Point<double> p) const;
        void changeLocationIfCloser(Parfait::Point<double> p, int new_index);
        double actualDistance() const;
        Parfait::Point<double> surfaceLocation() const;
        bool found() const;
        int facetIndex() const;

      private:
        Parfait::Point<double> query_point{0, 0, 0};
        Parfait::Point<double> surface_location{0, 0, 0};
        double actual_distance = std::numeric_limits<double>::max();
        int facet_index = -1;
        bool found_on_surface;
    };
    explicit DistanceTree(Parfait::Extent<double> e);

    void printVoxelStatistics();

    Parfait::Point<double> closestPoint(const Parfait::Point<double>& p,
                                        const Parfait::Point<double>& known_surface_location) const;
    Parfait::Point<double> closestPoint(const Parfait::Point<double>& p) const;
    std::tuple<Parfait::Point<double>, int> closestPointAndIndex(const Point<double>& p) const;
    std::tuple<Parfait::Point<double>, int> closestPointAndIndex(
        const Point<double>& p, const Parfait::Point<double>& known_surface_location) const;

  protected:
    CurrentState closestPoint(const Parfait::Point<double>& query_point,
                              const CurrentState& current_state,
                              PriorityQueue& process) const;

    CurrentState getClosestPointInLeaf(int voxel_index,
                                       const Point<double>& query_point,
                                       CurrentState current_state) const;

  private:
    size_t max_objects_per_voxel = 20;
};
}

#include "DistanceTree.hpp"