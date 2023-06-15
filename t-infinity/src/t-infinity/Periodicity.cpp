#include "Periodicity.h"
#include "FaceNeighbors.h"
#include <parfait/FileTools.h>
#include <parfait/PointWriter.h>
#include <parfait/ExtentWriter.h>
#include <parfait/ParallelExtent.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/VisualizePartition.h>
#include <t-infinity/Cell.h>
#include <parfait/Throw.h>

#define ASSERT_KEY(my_map, key) \
    PARFAIT_ASSERT(my_map.count(key) == 1, "Map key " + std::to_string(key) + " not found")

namespace inf {

namespace Periodic {

    inline std::vector<int> extractSurfaceNodeIdsInTagInSurfaceCells(const inf::MeshInterface& mesh,
                                                                     int tag) {
        std::set<int> node_ids;

        std::vector<int> cell_nodes;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.is2DCell(c)) {
                if (mesh.cellTag(c) == tag) {
                    mesh.cell(c, cell_nodes);
                    for (auto n : cell_nodes) {
                        node_ids.insert(n);
                    }
                }
            }
        }
        return std::vector<int>(node_ids.begin(), node_ids.end());
    }

    inline std::vector<int> extractOwnedSurfaceNodeIdsInTagInOwnedSurfaceCells(
        const inf::MeshInterface& mesh, int tag) {
        std::set<int> node_ids;

        std::vector<int> cell_nodes;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.is2DCell(c)) {
                if (mesh.cellTag(c) == tag) {
                    if (mesh.ownedCell(c)) {
                        mesh.cell(c, cell_nodes);
                        for (auto n : cell_nodes) {
                            if (mesh.ownedNode(n)) node_ids.insert(n);
                        }
                    }
                }
            }
        }
        return std::vector<int>(node_ids.begin(), node_ids.end());
    }

    Matcher::Matcher(MessagePasser mp,
                     Type periodic_type_in,
                     const inf::MeshInterface& mesh,
                     const std::vector<std::pair<int, int>>& tag_pairs,
                     double tolerance_)
        : mp(mp), periodic_type(periodic_type_in), match_tolerance(tolerance_) {
        for (const auto& pair : tag_pairs) {
            tags_in_play.insert(pair.first);
            tags_in_play.insert(pair.second);
            tag_to_pair[pair.first] = pair.second;
            tag_to_pair[pair.second] = pair.first;
        }

        for (auto t : tags_in_play) {
            nodes_in_tag[t] = extractSurfaceNodeIdsInTagInSurfaceCells(mesh, t);
            points_in_tag[t] = inf::extractPoints(mesh, nodes_in_tag[t]);
            Parfait::Point<double> centroid = {0, 0, 0};
            int count = 0;
            for (int c = 0; c < mesh.cellCount(); c++) {
                auto type = mesh.cellType(c);
                int tag = mesh.cellTag(c);
                if (mesh.cellOwner(c) != mesh.partitionId()) continue;
                if (tag != t) continue;
                if (not inf::MeshInterface::is2DCellType(type)) continue;
                inf::Cell cell(mesh, c);
                centroid += cell.faceAverageCenter(0);
                count++;
            }

            count = mp.ParallelSum(count);
            centroid[0] = mp.ParallelSum(centroid[0]);
            centroid[1] = mp.ParallelSum(centroid[1]);
            centroid[2] = mp.ParallelSum(centroid[2]);
            centroid /= double(count);
            tag_average_centroids[t] = centroid;
        }

        bool good = false;
        try {
            matchPeriodicNodes(mesh);
            findPeriodicFaceNeighbors(mesh);
            good = true;
        } catch (std::exception& e) {
            printf("Problem detected matching periodic faces:\n\n%s\n\n", e.what());
            good = false;
        }
        good = mp.ParallelAnd(good);
        if (not good) {
            auto ptr_mesh = std::make_shared<inf::TinfMesh>(mesh);
            mp_rootprint(
                "Problem detected attempting to match periodic faces.  Visualizing periodic "
                "surfaces: \n");
            std::vector<int> tags(tags_in_play.begin(), tags_in_play.end());
            inf::shortcut::visualize_surface_tags("periodic-surfaces", mp, ptr_mesh, tags, {}, {});
            PARFAIT_THROW("Error with periodic faces.");
        }
    }

    Parfait::Extent<double> Matcher::makeExtentMinimumLength(Parfait::Extent<double> e,
                                                             double min_length) {
        for (int i = 0; i < 3; i++) {
            if (e.lo[i] > e.hi[i]) {
                e.lo[i] = 0.0;
                e.hi[i] = 0.0;
            } else {
                e.lo[i] -= 0.5 * min_length;
                e.hi[i] += 0.5 * min_length;
            }
        }
        return e;
    }

    Parfait::Extent<double> Matcher::getMyExtentAroundTag(const inf::MeshInterface& mesh,
                                                          int tag) const {
        Parfait::Extent<double> e{{1, 1, 1}, {-1, -1, -1}};
        ASSERT_KEY(nodes_in_tag, tag);
        for (unsigned long index = 0; index < nodes_in_tag.at(tag).size(); index++) {
            ASSERT_KEY(nodes_in_tag, tag);
            int n = nodes_in_tag.at(tag)[index];
            if (mesh.ownedNode(n)) {
                ASSERT_KEY(points_in_tag, tag);
                auto p = points_in_tag.at(tag)[index];
                Parfait::ExtentBuilder::add(e, p);
            }
        }
        return e;
    }

    void Matcher::findPeriodicFaceNeighbors(const inf::MeshInterface& mesh) {
        auto node_g2l = inf::GlobalToLocal::buildNode(mesh);

        auto n2c = inf::NodeToCell::build(mesh);
        auto face_neighbors = inf::FaceNeighbors::buildOnlyForCellsWithDimensionality(mesh, 2);
        std::map<int, std::vector<PeriodicFace>> queries;

        std::vector<int> face_nodes;
        for (auto t : tags_in_play) {
            auto surface_face_ids = inf::extractOwnedCellIdsOnTag(mesh, t, 2);
            for (int c : surface_face_ids) {
                PeriodicFace periodic_face;
                PARFAIT_ASSERT(face_neighbors[c].size() == 1,
                               "Face neighbors should have precisely 1 volume neighbor");
                int volume_cell = face_neighbors[c].front();
                periodic_face.host_face_global_id = mesh.globalCellId(c);
                periodic_face.host_volume_global_id = mesh.globalCellId(volume_cell);
                periodic_face.owner_of_host_face = mesh.cellOwner(c);
                periodic_face.owner_of_host_volume = mesh.cellOwner(volume_cell);
                mesh.cell(c, face_nodes);
                std::set<int> ask_these_ranks;
                for (int n : face_nodes) {
                    long gid = mesh.globalNodeId(n);
                    Parfait::Point<double> p;
                    mesh.nodeCoordinate(n, p.data());
                    bool found_node = false;
                    if (global_node_to_periodic_nodes.count(gid) == 1) {
                        found_node = true;
                    }
                    if (not found_node) {
                        printf("Do not have periodic neighbor for node %ld\n", gid);
                        Parfait::Point<double> pp;
                        mesh.nodeCoordinate(n, pp.data());
                        Parfait::FileTools::appendToFile(
                            "periodic-face-not-found" + std::to_string(mp.Rank()) + ".3D",
                            p.to_string() + "\n");
                        continue;
                    }
                    PARFAIT_ASSERT(global_node_to_periodic_nodes.count(gid) == 1,
                                   "Could not find virtual node for global node " +
                                       std::to_string(gid) + " at " + p.to_string().c_str());
                    long virtual_node_count = global_node_to_periodic_nodes.at(gid).size();
                    if (virtual_node_count != 1) {
                        std::string message =
                            "A periodic node has multiple (" + std::to_string(virtual_node_count) +
                            ") virtual nodes. This should not happen with one periodicity type\n";
                        for (auto vnode : global_node_to_periodic_nodes.at(gid)) {
                            message += "host    rank: " + std::to_string(vnode.owner_of_host_node) +
                                       ", GID: " + std::to_string(vnode.host_global_node_id) + "\n";
                            message +=
                                "virtual rank: " + std::to_string(vnode.owner_of_virtual_node) +
                                ", GID: " + std::to_string(vnode.virtual_global_node_id) + "\n";
                        }
                        PARFAIT_WARNING(message);
                    }
                    PARFAIT_ASSERT(virtual_node_count == 1, "assuming only one virtual node");
                    // WHEN this assert triggers.  We need to know how to disambiguate what nodes
                    // make up this face on the neighboring virtual face. Right now I don't know how
                    // to do that.
                    periodic_face.host_face_nodes.push_back(gid);
                    auto periodic_node = global_node_to_periodic_nodes.at(gid).front();

                    long virtual_gid = periodic_node.virtual_global_node_id;
                    periodic_face.virtual_face_nodes.push_back(virtual_gid);
                    ask_these_ranks.insert(periodic_node.owner_of_virtual_node);
                }
                for (auto r : ask_these_ranks) {
                    queries[r].push_back(periodic_face);
                }
            }
        }
        auto packer = [](MessagePasser::Message& m, const PeriodicFace& f) {
            m.pack(f.host_face_global_id);
            m.pack(f.host_volume_global_id);
            m.pack(f.virtual_face_global_id);
            m.pack(f.virtual_volume_global_id);
            m.pack(f.owner_of_host_face);
            m.pack(f.owner_of_host_volume);
            m.pack(f.owner_of_virtual_face);
            m.pack(f.owner_of_virtual_volume);
            m.pack(f.host_face_nodes);
            m.pack(f.virtual_face_nodes);
        };
        auto unpacker = [](MessagePasser::Message& m, PeriodicFace& f) {
            m.unpack(f.host_face_global_id);
            m.unpack(f.host_volume_global_id);
            m.unpack(f.virtual_face_global_id);
            m.unpack(f.virtual_volume_global_id);
            m.unpack(f.owner_of_host_face);
            m.unpack(f.owner_of_host_volume);
            m.unpack(f.owner_of_virtual_face);
            m.unpack(f.owner_of_virtual_volume);
            m.unpack(f.host_face_nodes);
            m.unpack(f.virtual_face_nodes);
        };

        queries = mp.Exchange(queries, packer, unpacker);

        for (auto& pair : queries) {
            for (auto& face : pair.second) {
                face_nodes.clear();
                for (auto gid : face.virtual_face_nodes) {
                    debugVisualizeFace(mesh, node_g2l, face.virtual_face_nodes);
                    ASSERT_KEY(node_g2l, gid);
                    face_nodes.push_back(node_g2l.at(gid));
                }

                int neighbor = findFaceNeighbor(mesh, n2c, face_nodes);
                PARFAIT_ASSERT(face_neighbors[neighbor].size() == 1,
                               "Face neighbors should have precisely 1 neighbor.  Found: " +
                                   std::to_string(face_neighbors[neighbor].size()));
                int volume_neighbor = face_neighbors[neighbor].front();
                face.virtual_face_global_id = mesh.globalCellId(neighbor);
                face.virtual_volume_global_id = mesh.globalCellId(volume_neighbor);
                face.owner_of_virtual_face = mesh.cellOwner(neighbor);
                face.owner_of_virtual_volume = mesh.cellOwner(volume_neighbor);
            }
        }
        queries = mp.Exchange(queries, packer, unpacker);
        for (auto& pair : queries) {
            for (auto& face : pair.second) {
                faces[face.host_face_global_id] = face;
            }
        }
    }
    int Matcher::findFaceNeighbor(const MeshInterface& mesh,
                                  const std::vector<std::vector<int>>& n2c,
                                  const std::vector<int>& face_nodes) const {
        std::vector<int> candidates = n2c[face_nodes[0]];
        std::vector<int> surface_only_candidates;
        for (int c : candidates) {
            if (mesh.is2DCell(c)) surface_only_candidates.push_back(c);
        }
        PARFAIT_ASSERT(not surface_only_candidates.empty(),
                       "No suitable face neighbor was identified");
        auto neighbor =
            inf::FaceNeighbors::findFaceNeighbor(mesh, surface_only_candidates, face_nodes);
        if (neighbor == inf::FaceNeighbors::NEIGHBOR_OFF_RANK) {
            PARFAIT_THROW("Could not find periodic face neighbor");
        }
        return neighbor;
    }
    void Matcher::printPeriodicNodes() const {
        printf("There are %lu periodic host nodes\n", global_node_to_periodic_nodes.size());
        for (auto& pair : global_node_to_periodic_nodes) {
            printf(
                "Host node: gid: %ld has %ld virtual pair nodes\n", pair.first, pair.second.size());
            for (auto virtual_node : pair.second) {
                printf("  virtual node: gid %ld, points back to %ld, owned by: %d\n",
                       virtual_node.virtual_global_node_id,
                       virtual_node.host_global_node_id,
                       virtual_node.owner_of_virtual_node);
            }
        }
    }
    void Matcher::printPeriodicFaces() const {
        printf("There are %ld periodic faces\n", faces.size());
        for (auto& pair : faces) {
            auto& gid = pair.first;
            auto& f = pair.second;
            printf("  face gid: %ld, vol gid: %ld, virtual face gid: %ld virtual vol gid: %ld\n",
                   gid,
                   f.host_volume_global_id,
                   f.virtual_face_global_id,
                   f.virtual_volume_global_id);
        }
    }

    void Matcher::removeAnyExistingDebugFiles() {
        if (mp.Rank() == 0) {
            system("rm -rf periodic-*.3D");
        }
        mp.Barrier();
    }

    template <typename Container, typename Thing>
    bool contains(const Container& container, const Thing& thing) {
        for (const auto& t : container) {
            if (t == thing) return true;
        }
        return false;
    }

    void Matcher::matchPeriodicNodes(const inf::MeshInterface& mesh) {
        if (debug) {
            removeAnyExistingDebugFiles();
        }

        for (const auto& tag_pair : tag_to_pair) {
            auto tag1 = tag_pair.first;
            auto tag2 = tag_pair.second;
            auto e = getMyExtentAroundTag(mesh, tag2);
            ASSERT_KEY(tag_average_centroids, tag2);
            ASSERT_KEY(tag_average_centroids, tag1);
            auto point_mover = createMover(
                periodic_type, tag_average_centroids.at(tag1), tag_average_centroids.at(tag2));

            double fuzz = match_tolerance;
            e = makeExtentMinimumLength(e, fuzz);
            e.resize(1.01);
            auto everyones_tag_2_extents = mp.Gather(e);
            if (mp.Rank() == 0) {
                if (debug) {
                    std::vector<double> rank_ids(mp.NumberOfProcesses());
                    for (int i = 0; i < mp.NumberOfProcesses(); i++) {
                        rank_ids[i] = double(i);
                    }
                    Parfait::ExtentWriter::write(
                        "periodic-search-extents", everyones_tag_2_extents, rank_ids);
                }
            }

            std::map<int, std::vector<NodeQuery>> node_queries;
            ASSERT_KEY(nodes_in_tag, tag1);
            for (int local_id : nodes_in_tag.at(tag1)) {
                NodeQuery node_query;
                node_query.global_node_id = mesh.globalNodeId(local_id);
                mesh.nodeCoordinate(local_id, node_query.p.data());
                bool sent = false;
                auto p = node_query.p;
                node_query.p = point_mover(node_query.p);
                for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
                    if (everyones_tag_2_extents[rank].intersects(node_query.p)) {
                        node_queries[rank].push_back(node_query);
                        sent = true;
                    }
                }
                if (not sent and debug) {
                    printf("Did not send node %ld to any rank for searching",
                           node_query.global_node_id);
                    printf("Node lives on tag %d and should map to tag %d\n", tag1, tag2);
                    printf(
                        "plot this node: 0.0 is the original node point. 1.0 is the translated "
                        "point");
                    printf("# x y z\n");
                    Parfait::FileTools::appendToFile("periodic-not-sent.3D",
                                                     node_query.p.to_string() + " 1.0\n");
                    printf("%s 1.0\n", node_query.p.to_string().c_str());
                    printf("%s 0.0\n", node_query.p.to_string().c_str());
                    Parfait::FileTools::appendToFile("periodic-not-sent.3D",
                                                     p.to_string() + " 0.0\n");
                }
            }

            node_queries = mp.Exchange(node_queries);
            std::map<int, std::vector<PeriodicNode>> node_replies;

            ASSERT_KEY(nodes_in_tag, tag2);
            if (nodes_in_tag.at(tag2).size() != 0) {
                Parfait::Adt3DPoint adt(e);
                for (unsigned long index = 0; index < nodes_in_tag.at(tag2).size(); index++) {
                    int local_id = nodes_in_tag.at(tag2)[index];
                    long global_id = mesh.globalNodeId(local_id);
                    if (mesh.ownedNode(local_id)) {
                        Parfait::Point<double> p;
                        mesh.nodeCoordinate(local_id, p.data());
                        adt.store(global_id, p);
                    }
                }
                // printf("Rank %d has %d/%d of search nodes stored\n", mp.Rank(), stored_nodes,
                // int(nodes_in_tag.at(tag2).size()));

                for (auto& pair : node_queries) {
                    int query_rank = pair.first;
                    auto& queries = pair.second;
                    for (auto q : queries) {
                        int paired_global_node_id_as_int;
                        if (adt.isPointInAdt(q.p, paired_global_node_id_as_int, match_tolerance)) {
                            PeriodicNode reply;
                            reply.host_global_node_id = q.global_node_id;
                            reply.owner_of_host_node = query_rank;
                            reply.virtual_global_node_id = long(paired_global_node_id_as_int);
                            reply.owner_of_virtual_node = mp.Rank();
                            node_replies[query_rank].push_back(reply);
                        }
                    }
                }
            }

            node_replies = mp.Exchange(node_replies);
            for (auto& pair : node_replies) {
                for (auto& periodic_node : pair.second) {
                    if (not contains(
                            global_node_to_periodic_nodes[periodic_node.host_global_node_id],
                            periodic_node)) {
                        global_node_to_periodic_nodes[periodic_node.host_global_node_id].push_back(
                            periodic_node);
                    }
                }
            }
        }
        debugCheckWeFoundPeriodicNodesForAllTheNodesWeSearchedFor(mesh);
        //    debugPrintPeriodicNodes();
    }

    void Matcher::debugCheckWeFoundPeriodicNodesForAllTheNodesWeSearchedFor(
        const inf::MeshInterface& mesh) {
        std::map<int, bool> found_node_pair;

        for (auto& t : tags_in_play) {
            for (auto n : nodes_in_tag[t]) {
                auto gid = mesh.globalNodeId(n);
                if (found_node_pair.count(n) == 0) {
                    found_node_pair[n] = false;
                }
                if (global_node_to_periodic_nodes.count(gid)) {
                    if (global_node_to_periodic_nodes.at(gid).size() != 0)
                        found_node_pair[n] = true;
                }
            }
        }

        bool good = true;
        int not_found_count = 0;
        for (auto& pair : found_node_pair) {
            int n = pair.first;
            if (debug) {
                Parfait::Point<double> p;
                mesh.nodeCoordinate(n, p.data());
                Parfait::FileTools::appendToFile(
                    "periodic-searched-and-found." + std::to_string(mp.Rank()) + ".3D",
                    p.to_string() + "\n");
            }
            bool found = pair.second;
            if (not found) {
                good = false;
                Parfait::Point<double> p;
                mesh.nodeCoordinate(n, p.data());
                long gid = mesh.globalNodeId(n);
                printf("rank %d Did not find node %ld, %s to any rank for searching\n",
                       mp.Rank(),
                       gid,
                       p.to_string().c_str());
                Parfait::FileTools::appendToFile("periodic-not-found.3D", p.to_string() + "\n");
                not_found_count++;
            }
        }
        if (not good) {
            printf("rank %d did not find %d nodes\n", mp.Rank(), not_found_count);
        }
        good = mp.ParallelAnd(good);
        PARFAIT_ASSERT(good, "Did not find all periodic nodes virtual neighbors");
    }
    void Matcher::debugVisualizeFace(const MeshInterface& mesh,
                                     const std::map<long, int>& node_g2l,
                                     const std::vector<long>& face_nodes) {
        bool all_found = true;
        std::vector<Parfait::Point<double>> points;
        for (auto node_gid : face_nodes) {
            if (node_g2l.count(node_gid) == 0) {
                all_found = false;
                printf("Rank %d cound not find global node %lu\n", mesh.partitionId(), node_gid);
            } else {
                Parfait::Point<double> p;
                mesh.nodeCoordinate(node_g2l.at(node_gid), p.data());
                points.push_back(p);
            }
        }

        if (not all_found) {
            auto hash = Parfait::StringTools::randomLetters(5);
            Parfait::PointWriter::write(
                "periodic-face." + std::to_string(mesh.partitionId()) + "." + hash, points);
        }
    }
    void Matcher::debugPrintPeriodicNodes() {
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            if (mp.Rank() == r) {
                for (auto& pair : global_node_to_periodic_nodes) {
                    printf("Global Node %ld, virtual node %ld owner %d\n",
                           pair.first,
                           pair.second.front().virtual_global_node_id,
                           pair.second.front().owner_of_virtual_node);
                }
            }
            mp.Barrier();
        }
    }
    std::function<Parfait::Point<double>(const Parfait::Point<double>&)> createMover(
        Type type, const Parfait::Point<double>& from, const Parfait::Point<double>& to) {
        auto from_2d = from;
        auto to_2d = to;
        switch (type) {
            case TRANSLATION: {
                auto offset = to - from;
                auto mover = [=](const Parfait::Point<double>& p) { return p + offset; };
                return mover;
            }
            case AXISYMMETRIC_X: {
                from_2d[0] = 0;
                to_2d[0] = 0;
                auto magnitude = from_2d.magnitude() * to_2d.magnitude();
                auto dot = Parfait::Point<double>::dot(from_2d, to_2d);
                auto cross = Parfait::Point<double>::cross(from_2d, to_2d);
                auto angle = acos(dot / magnitude);
                if (Parfait::Point<double>::dot({1, 0, 0}, cross) < 0) {
                    angle = -angle;
                }
                auto mover = [=](const Parfait::Point<double>& p) {
                    auto o = p;
                    o[1] = cos(angle) * p[1] - sin(angle) * p[2];
                    o[2] = sin(angle) * p[1] + cos(angle) * p[2];
                    return o;
                };
                return mover;
            }

            case AXISYMMETRIC_Y: {
                from_2d[1] = 0;
                to_2d[1] = 0;
                auto magnitude = from_2d.magnitude() * to_2d.magnitude();
                auto dot = Parfait::Point<double>::dot(from_2d, to_2d);
                auto cross = Parfait::Point<double>::cross(from_2d, to_2d);
                auto angle = acos(dot / magnitude);
                if (Parfait::Point<double>::dot({0, 1, 0}, cross) < 0) {
                    angle = -angle;
                }
                auto mover = [=](const Parfait::Point<double>& p) {
                    auto o = p;
                    o[2] = cos(angle) * p[2] - sin(angle) * p[0];
                    o[0] = sin(angle) * p[2] + cos(angle) * p[0];
                    return o;
                };
                return mover;
            }
            case AXISYMMETRIC_Z: {
                from_2d[2] = 0;
                to_2d[2] = 0;
                auto magnitude = from_2d.magnitude() * to_2d.magnitude();
                auto dot = Parfait::Point<double>::dot(from_2d, to_2d);
                auto cross = Parfait::Point<double>::cross(from_2d, to_2d);
                auto angle = acos(dot / magnitude);
                if (Parfait::Point<double>::dot({0, 0, 1}, cross) < 0) {
                    angle = -angle;
                }
                auto mover = [=](const Parfait::Point<double>& p) {
                    auto o = p;
                    o[0] = cos(angle) * p[0] - sin(angle) * p[1];
                    o[1] = sin(angle) * p[0] + cos(angle) * p[1];
                    return o;
                };
                return mover;
            }
            default:
                PARFAIT_THROW("Do not support Periodic::Type " + std::to_string(type));
        }
    }
}
}
