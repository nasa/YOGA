#pragma once
#include <parfait/Barycentric.h>
#include <parfait/Point.h>
#include <parfait/Facet.h>
#include <parfait/Adt3dExtent.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/Cell.h>
#include <t-infinity/FieldInterface.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/Throw.h>
#include "FaceNeighbors.h"
#include <parfait/DataFrame.h>

namespace inf {
namespace linesampling {

    Parfait::DataFrame sample(MessagePasser mp,
                              const MeshInterface& mesh,
                              Parfait::Point<double> a,
                              Parfait::Point<double> b,
                              const std::vector<std::vector<double>>& node_fields = {},
                              const std::vector<std::string>& field_names = {});

    Parfait::DataFrame sample(MessagePasser mp,
                              const MeshInterface& mesh,
                              const std::vector<std::vector<int>>& face_neighbors,
                              Parfait::Point<double> a,
                              Parfait::Point<double> b,
                              const std::vector<std::vector<double>>& node_fields = {},
                              const std::vector<std::string>& field_names = {});
    
    Parfait::DataFrame sample(MessagePasser mp,
                              const MeshInterface& mesh,
                              const std::vector<std::vector<int>>& face_neighbors,
                              const std::vector<int>& cell_id_set,
                              Parfait::Point<double> a,
                              Parfait::Point<double> b,
                              const std::vector<std::vector<double>>& node_fields = {},
                              const std::vector<std::string>& field_names = {});
    
    Parfait::DataFrame sample(MessagePasser mp,
                              const MeshInterface& mesh,
                              const std::vector<std::vector<int>>& face_neighbors,
                              const Parfait::Adt3DExtent& adt,
                              Parfait::Point<double> a,
                              Parfait::Point<double> b,
                              const std::vector<std::vector<double>>& node_fields = {},
                              const std::vector<std::string>& field_names = {});

    void visualize(std::string filename,
                   MessagePasser mp,
                   const MeshInterface& mesh,
                   Parfait::Point<double> a,
                   Parfait::Point<double> b,
                   const std::vector<std::shared_ptr<FieldInterface>>& fields);

    class Cut {
      public:
        inline Cut(const MeshInterface& m, Parfait::Point<double> a, Parfait::Point<double> b)
            : mesh(m), line_a(a), line_b(b) {
            std::vector<std::vector<int>> face_neighbors = FaceNeighbors::build(mesh);
            std::vector<int> cell_id_set;
            for (int c = 0; c < mesh.cellCount(); c++) cell_id_set.push_back(c);
            performCut(face_neighbors, cell_id_set);
        }
        inline Cut(const MeshInterface& m,
                   const std::vector<std::vector<int>>& face_neighbors,
                   Parfait::Point<double> a,
                   Parfait::Point<double> b)
            : mesh(m), line_a(a), line_b(b) {
            std::vector<int> cell_id_set;
            for (int c = 0; c < mesh.cellCount(); c++) cell_id_set.push_back(c);
            performCut(face_neighbors, cell_id_set);
        }
        inline Cut(const MeshInterface& m,
                   const std::vector<std::vector<int>>& face_neighbors,
                   const std::vector<int>& cell_id_set,
                   Parfait::Point<double> a,
                   Parfait::Point<double> b) 
            : mesh(m), line_a(a), line_b(b) {
            performCut(face_neighbors, cell_id_set);
        }

        inline int nodeCount() const { return node_weights.size(); }

        std::vector<Parfait::Point<double>> getSampledPoints() const;
        std::vector<double> apply(const std::vector<double>& f) const;
        std::shared_ptr<inf::FieldInterface> apply(const inf::FieldInterface& f) const;

      private:
        const MeshInterface& mesh;
        Parfait::Point<double> line_a;
        Parfait::Point<double> line_b;

        std::vector<std::array<double, 3>> node_weights;
        std::vector<std::array<int, 3>> node_donor_nodes;
        std::vector<double> node_t_value;

        void addTriIfIntersects(int node0, int node1, int node2);
        void performCut(const std::vector<std::vector<int>>& face_neighbors,
                        const std::vector<int>& cell_id_set);
    };
}
}