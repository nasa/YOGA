#pragma once
#include <array>
#include <vector>
#include <map>
#include <parfait/Point.h>
#include <parfait/CGNSElements.h>
#include <MessagePasser/MessagePasser.h>
#include <memory>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <parfait/Throw.h>

namespace inf {
namespace isosampling {

    std::function<double(int)> sphere(const inf::MeshInterface& mesh,
                                      Parfait::Point<double> center,
                                      double radius);
    std::function<double(int)> plane(const inf::MeshInterface& mesh,
                                     Parfait::Point<double> center,
                                     Parfait::Point<double> normal);

    std::function<double(int)> field_isosurface(const inf::FieldInterface& field, double value);

    class CutEdge {
      public:
        int a, b;
        double w;

      public:
        CutEdge(int first, int second, double weight);
    };

    class Isosurface {
      public:
        Isosurface(MessagePasser mp,
                   std::shared_ptr<inf::MeshInterface> mesh,
                   std::function<double(int)> get_nodal_iso_value);

        std::shared_ptr<inf::TinfMesh> getMesh() const;

        std::shared_ptr<inf::FieldInterface> apply(const inf::FieldInterface& f);
        inline std::shared_ptr<inf::FieldInterface> apply(const inf::FieldInterface* f) {
            return apply(*f);
        }
        inline std::shared_ptr<inf::FieldInterface> apply(std::shared_ptr<inf::FieldInterface> f) {
            return apply(*f);
        }

      public:
        MessagePasser mp;
        std::shared_ptr<inf::MeshInterface> mesh;
        std::map<std::pair<int, int>, int> edge_number;
        std::vector<double> edge_weights;
        std::vector<std::array<int, 2>> edge_nodes;
        std::vector<std::array<int, 3>> tri_edges;
        std::vector<std::array<int, 2>> bar_edges;
        int add(const CutEdge& edge);
        int addTri(const CutEdge& a, const CutEdge& b, const CutEdge& c);
        int addBar(const CutEdge& a, const CutEdge& b);
    };
}
}