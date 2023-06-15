#pragma once
#include <memory>
#include <limits>
#include <parfait/Point.h>
#include <parfait/Throw.h>

namespace inf {
namespace geometry {

    class EdgeInterface {
      public:
        virtual Parfait::Point<double> project(Parfait::Point<double> p) const = 0;
        virtual ~EdgeInterface() = default;
    };

    class FaceInterface {
      public:
        virtual Parfait::Point<double> project(Parfait::Point<double> p) const = 0;
        virtual ~FaceInterface() = default;
    };

    class DatabaseInterface {
      public:
        virtual std::shared_ptr<EdgeInterface> getEdge(int edge_id) = 0;
        virtual std::shared_ptr<FaceInterface> getFace(int face_id) = 0;
        virtual Parfait::Point<double> getPoint(int point_id) = 0;
        virtual ~DatabaseInterface() = default;
    };

    struct GeomAssociation {
        enum Type { NODE, EDGE, FACE };
        Type type;
        int index;
    };

}
}

namespace inf {
namespace geom_impl {
    class PiecewiseP1Edge : public geometry::EdgeInterface {
      public:
        inline PiecewiseP1Edge(std::vector<Parfait::Point<double>> ordered_points)
            : points(ordered_points) {}

        inline Parfait::Point<double> project(Parfait::Point<double> p) const override {
            int num_segments = points.size() - 1;
            double closest_distance = std::numeric_limits<double>::max();
            auto closest_point = p;
            for (int i = 0; i < num_segments; i++) {
                auto potential_point = projectLinearOnSegment(i, p);
                auto dist = p.distance(potential_point);
                if (dist < closest_distance) {
                    closest_point = potential_point;
                    closest_distance = dist;
                }
            }
            return closest_point;
        }

      public:
        std::vector<Parfait::Point<double>> points;

        inline Parfait::Point<double> projectLinearOnSegment(int segment,
                                                             Parfait::Point<double> p) const {
            auto a = points[segment];
            auto b = points[segment + 1];
            auto ab = b - a;
            auto ap = p - a;
            double length = ab.magnitude();
            double t = ap.dot(ab) / (length * length);
            if (t < 0) t = 0;
            if (t > 1.0) t = 1.0;
            return a + t * ab;
        }
    };

    class PiecewiseP1DataBase : public geometry::DatabaseInterface {
      public:
        std::shared_ptr<geometry::EdgeInterface> getEdge(int edge_id) override {
            return curves.at(edge_id);
        }
        std::shared_ptr<geometry::FaceInterface> getFace(int face_id) override { return nullptr; }
        Parfait::Point<double> getPoint(int point_id) override { return {0, 0, 0}; }

      public:
        std::vector<std::shared_ptr<geom_impl::PiecewiseP1Edge>> curves;
    };
}
}
