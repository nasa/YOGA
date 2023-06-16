#pragma once
#include <t-infinity/Extract.h>
#include <t-infinity/Extract.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/Adt3dPoint.h>
#include <parfait/Throw.h>

namespace inf {

namespace Periodic {
    enum Type { TRANSLATION = 0, AXISYMMETRIC_X = 1, AXISYMMETRIC_Y = 2, AXISYMMETRIC_Z = 3 };

    class Matcher {
      public:
        struct NodeQuery {
            long global_node_id;
            Parfait::Point<double> p;
        };

        struct PeriodicNode {
            long host_global_node_id;
            long virtual_global_node_id;
            int owner_of_virtual_node;
            int owner_of_host_node;
            inline bool operator==(const PeriodicNode& rhs) const {
                if (host_global_node_id != rhs.host_global_node_id) return false;
                if (virtual_global_node_id != rhs.virtual_global_node_id) return false;
                if (owner_of_virtual_node != rhs.owner_of_virtual_node) return false;
                if (owner_of_host_node != rhs.owner_of_host_node) return false;
                return true;
            }
        };

        struct PeriodicFace {
            long host_face_global_id;
            long host_volume_global_id;
            long virtual_face_global_id;
            long virtual_volume_global_id;
            int owner_of_host_face;
            int owner_of_host_volume;
            int owner_of_virtual_face;
            int owner_of_virtual_volume;
            std::vector<long> host_face_nodes;
            std::vector<long> virtual_face_nodes;
        };

        Matcher(MessagePasser mp,
                Type periodic_type,
                const inf::MeshInterface& mesh,
                const std::vector<std::pair<int, int>>& tag_pairs,
                double tolerance = 1.0e-8);

        static Parfait::Extent<double> makeExtentMinimumLength(Parfait::Extent<double> e,
                                                               double min_length);

        Parfait::Extent<double> getMyExtentAroundTag(const MeshInterface& mesh, int tag) const;

      public:
        MessagePasser mp;
        Type periodic_type;
        bool debug = false;
        double match_tolerance = 1.0e-6;
        std::set<int> tags_in_play;
        std::map<int, int> tag_to_pair;
        std::map<int, std::vector<int>> nodes_in_tag;
        std::map<int, std::vector<Parfait::Point<double>>> points_in_tag;
        std::map<int, Parfait::Point<double>> tag_average_centroids;
        std::map<long, std::vector<PeriodicNode>> global_node_to_periodic_nodes;
        std::map<long, PeriodicFace> faces;
        void findPeriodicFaceNeighbors(const inf::MeshInterface& mesh);
        int findFaceNeighbor(const MeshInterface& mesh,
                             const std::vector<std::vector<int>>& n2c,
                             const std::vector<int>& face_nodes) const;

        void printPeriodicNodes() const;
        void printPeriodicFaces() const;
        void matchPeriodicNodes(const inf::MeshInterface& mesh);

        void removeAnyExistingDebugFiles();
        void debugCheckWeFoundPeriodicNodesForAllTheNodesWeSearchedFor(
            const inf::MeshInterface& mesh);
        void debugVisualizeFace(const MeshInterface& mesh,
                                const std::map<long, int>& node_g2l,
                                const std::vector<long>& face_nodes);
        void debugPrintPeriodicNodes();
    };

    std::function<Parfait::Point<double>(const Parfait::Point<double>&)> createMover(
        Type type, const Parfait::Point<double>& from, const Parfait::Point<double>& to);

}
}
