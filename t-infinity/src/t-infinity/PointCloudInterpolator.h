#pragma once
#include <t-infinity/MeshInterface.h>
#include "parfait/Adt3dPoint.h"
#include <t-infinity/Cell.h>
#include <parfait/Point.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/ExtentWriter.h>
#include <parfait/Adt3dExtent.h>
#include <parfait/PointWriter.h>
#include <parfait/Interpolation.h>
#include <parfait/CellContainmentChecker.h>
#include <t-infinity/Extract.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/MeshConnectivity.h>
#include "t-infinity/MeshGatherer.h"
#include <t-infinity/InterpolationInterface.h>
#include <parfait/Dictionary.h>
#include <parfait/FileTools.h>
#include <parfait/OmlParser.h>

namespace inf {
class Interpolator : public inf::InterpolationInterface {
    enum SearchPolicy { STRICT, EXHAUSTIVE };
    enum DonorType : int { ORPHAN = 0, AABB_OVERLAP = 1, FOUND_VIA_SEARCH = 2 };
    SearchPolicy search_policy = STRICT;

  public:
    inline Interpolator(MessagePasser mp,
                        std::shared_ptr<inf::MeshInterface> donor_mesh,
                        std::shared_ptr<inf::MeshInterface> receptor_point_cloud_in)
        : mp(mp),
          donor_mesh(donor_mesh),
          receptor_point_cloud(receptor_point_cloud_in),
          donor_n2c(inf::NodeToCell::build(*donor_mesh)) {
        ids_of_donor_volume_cells = extractIdsForVolumeCells(*donor_mesh);
        ids_of_receptor_nodes = extractIdsForAllNodes(*receptor_point_cloud);

        Parfait::Dictionary settings;
        donor_patterns = buildDonorPatterns();
        reportOrphans();
    }
    inline void addDebugFields(std::vector<std::shared_ptr<inf::FieldInterface>>& fields) override {
        auto node_status = getReceptorNodeStatus();
        auto node_status_field = std::make_shared<inf::VectorFieldAdapter>(
            "interpolator-node-status", FieldAttributes::Node(), node_status);
        fields.push_back(node_status_field);
    }
    std::vector<double> getReceptorNodeStatus() {
        std::vector<double> receptor_node_status(receptor_point_cloud->nodeCount(), 0);
        for (int n = 0; n < receptor_point_cloud->nodeCount(); n++) {
            if (donor_patterns.count(n) == 0) {
                receptor_node_status[n] = ORPHAN;
            } else {
                receptor_node_status[n] = donor_patterns.at(n).donor_type;
            }
        }
        return receptor_node_status;
    }

    void reportOrphans() {
        int orphan_nodes = 0;
        for (int n = 0; n < receptor_point_cloud->nodeCount(); n++) {
            if (receptor_point_cloud->nodeOwner(n) == mp.Rank()) {
                if (donor_patterns.count(n) == 0) {
                    Parfait::Point<double> p = receptor_point_cloud->node(n);
                    printf("Found orphan %d: %s\n", n, p.to_string().c_str());
                    orphan_nodes++;
                }
            }
        }
        orphan_nodes = mp.ParallelSum(orphan_nodes);
        if (orphan_nodes != 0) {
            mp_rootprint("WARNING!  %d Orphan locations found!\n", orphan_nodes);
            mp_rootprint(
                "WARNING!  These are regions in the target mesh with no suitable\n"
                "WARNING!  interpolation could be found in the source mesh\n");
            mp_rootprint(
                "WARNING!  This could be caused if there is a major geometric change between the "
                "two meshes\n");
            mp_rootprint(
                "WARNING!  Please contact Matthew.D.OConnell@nasa.gov\n"
                "WARNING!  There are additional techniques that we can try\n");
        }
    }

    std::shared_ptr<inf::FieldInterface> interpolateMulti(
        std::shared_ptr<inf::FieldInterface> f) const {
        int num_receptors = 0;
        if (f->association() == inf::FieldAttributes::Node())
            num_receptors = receptor_point_cloud->nodeCount();
        if (f->association() == inf::FieldAttributes::Cell())
            num_receptors = receptor_point_cloud->cellCount();
        int bs = f->blockSize();
        std::vector<double> all_interpolated(num_receptors * f->blockSize());

        for (int c = 0; c < f->blockSize(); c++) {
            auto column = inf::FieldTools::extractColumnAsField(*f, c);
            auto interpolated_column = interpolate(column);
            for (int n = 0; n < num_receptors; n++) {
                all_interpolated[n * bs + c] = interpolated_column->getDouble(n, 0);
            }
        }

        return std::make_shared<inf::VectorFieldAdapter>(
            f->name(), f->association(), bs, all_interpolated);
    }

    std::shared_ptr<inf::FieldInterface> interpolate(
        std::shared_ptr<inf::FieldInterface> f) const override {
        std::string input_association = f->association();

        if (f->blockSize() > 1) return interpolateMulti(f);

        if (f->association() == inf::FieldAttributes::Cell()) {
            f = inf::FieldTools::cellToNode_volume(*donor_mesh, *f, donor_n2c);
        }

        auto f_vector = inf::FieldTools::extractColumn(*f, 0);

        auto f_at_target_nodes = interpolateFromNodeData(f_vector);

        std::shared_ptr<inf::FieldInterface> f_interpolated =
            std::make_shared<inf::VectorFieldAdapter>(
                f->name(), inf::FieldAttributes::Node(), 1, f_at_target_nodes);
        if (input_association == inf::FieldAttributes::Cell()) {
            f_interpolated = inf::FieldTools::nodeToCell(*receptor_point_cloud, *f_interpolated);
        }
        return f_interpolated;
    }

    std::vector<double> interpolateFromNodeData(const std::vector<double>& field) const {
        // build request
        std::map<int, std::vector<DonorPattern>> requests_before_packing;
        for (auto& pair : donor_patterns) {
            auto& pattern = pair.second;
            requests_before_packing[pattern.host_rank].push_back(pattern);
        }
        // pack
        std::map<int, MessagePasser::Message> requests;
        for (auto& pair : requests_before_packing) {
            int num_patterns = int(pair.second.size());
            requests[pair.first].pack(num_patterns);
            for (auto& p : pair.second) {
                p.pack(requests[pair.first]);
            }
        }

        // exchange
        auto incoming_requests = mp.Exchange(requests);
        requests.clear();

        // do interpolation
        std::map<int, std::vector<DonorReply>> interpolated_replies;
        std::vector<Parfait::Point<double>> stencil_points;
        std::vector<double> stencil_values;
        for (auto& pair : incoming_requests) {
            int receptor_rank = pair.first;
            int num_patterns = -1;
            pair.second.unpack(num_patterns);
            for (int j = 0; j < num_patterns; j++) {
                DonorPattern pattern;
                pattern.unpack(pair.second);
                DonorReply r;
                r.id = pattern.receptor_id;
                r.value = 0;
                stencil_points.clear();
                stencil_values.clear();

                bool skip = false;
                for (int i = 0; i < int(pattern.nodes_on_host_rank.size()); i++) {
                    int id = pattern.nodes_on_host_rank[i];
                    Parfait::Point<double> p = donor_mesh->node(id);
                    double v = field[id];
                    if (p.approxEqual(pattern.query_point)) {
                        r.value = v;
                        skip = true;
                        break;
                    }
                    stencil_points.push_back(p);
                    stencil_values.push_back(v);
                }
                if (not skip) {
                    r.value = Parfait::interpolate_strict(stencil_points.size(),
                                                          (double*)stencil_points.data(),
                                                          stencil_values.data(),
                                                          pattern.query_point.data());
                }
                interpolated_replies[receptor_rank].push_back(r);
            }
        }

        // exchange
        auto incoming_replies = mp.Exchange(interpolated_replies);

        std::vector<double> out_field(receptor_point_cloud->nodeCount());
        for (auto& pair : incoming_replies) {
            for (auto& r : pair.second) {
                out_field[r.id] = r.value;
            }
        }
        return out_field;
    }

  private:
    struct DonorPattern {
        int host_rank;
        int receptor_id;
        DonorType donor_type;
        double distance_to_donor_cell;
        Parfait::Point<double> query_point;
        std::vector<int> nodes_on_host_rank;

        void pack(MessagePasser::Message& m) {
            m.pack(host_rank);
            m.pack(receptor_id);
            m.pack(donor_type);
            m.pack(query_point);
            m.pack(distance_to_donor_cell);
            m.pack(nodes_on_host_rank);
        }

        void unpack(MessagePasser::Message& m) {
            m.unpack(host_rank);
            m.unpack(receptor_id);
            m.unpack(donor_type);
            m.unpack(query_point);
            m.unpack(distance_to_donor_cell);
            m.unpack(nodes_on_host_rank);
        };
    };
    struct DonorCandidate {
        int id;
        double distance;
    };
    struct SearchPoint {
        int id;
        Parfait::Point<double> p;
    };
    struct DonorReply {
        int id;
        double value;
    };

    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> donor_mesh;
    std::shared_ptr<inf::MeshInterface> receptor_point_cloud;
    std::vector<std::vector<int>> donor_n2c;
    std::vector<int> ids_of_donor_volume_cells;
    std::vector<int> ids_of_receptor_nodes;
    std::vector<Parfait::Extent<double>> extent_of_each_partition;
    std::map<int, DonorPattern> donor_patterns;
    double tol = 1.0e-8;

    std::shared_ptr<Parfait::Adt3DExtent> createADTFromCells() const {
        using namespace Parfait;
        auto mesh_extent = ExtentBuilder::createEmptyBuildableExtent<double>();
        for (int i = 0; i < donor_mesh->nodeCount(); i++) {
            Parfait::Point<double> p = donor_mesh->node(i);
            ExtentBuilder::add(mesh_extent, p);
        }
        auto t = std::make_shared<Parfait::Adt3DExtent>(mesh_extent);
        auto extents = extractCellExtents();
        int num_donors = int(ids_of_donor_volume_cells.size());
        for (int i = 0; i < num_donors; i++) {
            t->store(ids_of_donor_volume_cells[i], extents[i]);
        }
        mp.Barrier();
        return t;
    }

    std::vector<Parfait::Extent<double>> extractCellExtents() const {
        using namespace Parfait;
        std::vector<int> cell;
        std::vector<Extent<double>> extents;
        for (int cell_id : ids_of_donor_volume_cells) {
            donor_mesh->cell(cell_id, cell);
            auto cell_extent = ExtentBuilder::createEmptyBuildableExtent<double>();
            for (int node : cell) {
                Parfait::Point<double> p = donor_mesh->node(node);
                ExtentBuilder::add(cell_extent, p);
            }
            extents.push_back(cell_extent);
        }
        return extents;
    }

    std::vector<int> extractIdsForAllCells(const inf::MeshInterface& mesh) const {
        std::vector<int> ids(mesh.cellCount());
        std::iota(ids.begin(), ids.end(), 0);
        return ids;
    }
    std::vector<int> extractIdsForAllNodes(const MeshInterface& mesh) {
        std::vector<int> ids(mesh.nodeCount());
        std::iota(ids.begin(), ids.end(), 0);
        return ids;
    }

    std::vector<int> extractIdsForVolumeCells(const inf::MeshInterface& mesh) const {
        std::vector<int> ids;
        for (int i = 0; i < mesh.cellCount(); i++) {
            if (mesh.is3DCellType(mesh.cellType(i))) {
                if (mesh.cellOwner(i) == mp.Rank()) ids.push_back(i);
            }
        }
        return ids;
    }

    DonorCandidate pickBestCandidate(const std::vector<int>& canidate_cells,
                                     const Parfait::Point<double>& query_point) {
        Parfait::Point<double> fuzz{tol, tol, tol};
        DonorCandidate best_donor;
        best_donor.id = -1;
        best_donor.distance = std::numeric_limits<double>::max();

        std::vector<int> cell;

        for (int c : canidate_cells) {
            donor_mesh->cell(c, cell);
            if (isInCell(cell, query_point)) {
                best_donor.id = c;
                best_donor.distance = 0.0;
                return best_donor;
            } else {
                double d = calcDistanceToDonor(cell, query_point);
                if (d < best_donor.distance) {
                    best_donor.id = c;
                    best_donor.distance = d;
                }
            }
        }
        return best_donor;
    }

    std::map<int, std::vector<SearchPoint>> buildSendQueries(
        const std::vector<int>& nodes_to_search_for) {
        std::map<int, std::vector<SearchPoint>> send_queries;

        for (auto node_id : nodes_to_search_for) {
            Parfait::Point<double> p = receptor_point_cloud->node(node_id);
            bool found = false;
            for (int r = 0; r < mp.NumberOfProcesses(); r++) {
                if (extent_of_each_partition[r].intersects(p)) {
                    send_queries[r].push_back({node_id, p});
                    found = true;
                }
            }

            if (not found and search_policy == EXHAUSTIVE) {
                int closest_partition = 0;
                auto minimum_distance = std::numeric_limits<double>::max();
                for (int r = 0; r < mp.NumberOfProcesses(); ++r) {
                    auto d = extent_of_each_partition[r].distance(p);
                    if (d < minimum_distance) {
                        minimum_distance = d;
                        closest_partition = r;
                    }
                }
                send_queries[closest_partition].push_back({node_id, p});
            }
        }

        return send_queries;
    }

    std::vector<int> findCandidates(Parfait::Adt3DExtent& tree, const Parfait::Point<double>& p) {
        double search_radius = tol;
        std::vector<int> candidates;
        for (int i = 0; i < 10; i++) {
            Parfait::Point<double> fuzz = {search_radius, search_radius, search_radius};
            auto e = Parfait::Extent<double>{p - fuzz, p + fuzz};
            candidates = tree.retrieve(e);
            if (candidates.size() != 0) {
                break;
            }

            search_radius *= 2;
        }
        return candidates;
    }

    int aNotGreatWayToFindNearestCellForExtrapolation(const Parfait::Adt3DExtent& tree,
                                                      SearchPoint& search_point,
                                                      double starting_size) {
        Parfait::Extent<double> e(search_point.p, search_point.p);
        Parfait::Point<double> tolerance(starting_size, starting_size, starting_size);
        for (int i = 0; i < 100; ++i) {
            e.lo -= tolerance;
            e.hi += tolerance;
            tolerance *= 1.2;
            auto ids = tree.retrieve(e);
            if (not ids.empty()) {
                std::vector<int> node_ids;
                int clostest_cell_id = 0;
                double min_distance = std::numeric_limits<double>::max();
                for (int cell_id : ids) {
                    donor_mesh->cell(cell_id, node_ids);
                    for (int node_id : node_ids) {
                        Parfait::Point<double> p = donor_mesh->node(node_id);
                        auto d = Parfait::Point<double>::distance(search_point.p, p);
                        if (d < min_distance) {
                            clostest_cell_id = cell_id;
                            min_distance = d;
                        }
                    }
                }
                return clostest_cell_id;
            }
        }
        return -1;
    }

    std::map<int, MessagePasser::Message> buildReplies(
        std::shared_ptr<Parfait::Adt3DExtent> tree,
        std::map<int, std::vector<SearchPoint>>& incoming_queries) {
        std::map<int, std::vector<DonorPattern>> donor_pattern_replies;

        mp_rootprint("Searching tree.  This may take a while\n");
        auto domain_extents = mp.Gather(tree->boundingExtent());
        auto domain_of_the_whole_extent = domain_extents.front();
        for (const auto& e : domain_extents) {
            Parfait::ExtentBuilder::add(domain_of_the_whole_extent, e);
        }
        double starting_size = 0.001 * domain_of_the_whole_extent.radius();
        for (auto& pair : incoming_queries) {
            int source_rank = pair.first;
            for (auto search_point : pair.second) {
                auto candidates = findCandidates(*tree, search_point.p);
                DonorPattern pattern;
                auto best = pickBestCandidate(candidates, search_point.p);
                pattern.donor_type = AABB_OVERLAP;
                if (best.id == -1 and search_policy == EXHAUSTIVE) {
                    best.id = aNotGreatWayToFindNearestCellForExtrapolation(
                        *tree, search_point, starting_size);
                    pattern.donor_type = FOUND_VIA_SEARCH;
                }
                if (best.id == -1) continue;  // Extrapolation failure
                pattern.host_rank = mp.Rank();
                pattern.receptor_id = search_point.id;
                donor_mesh->cell(best.id, pattern.nodes_on_host_rank);
                int cell_length = int(pattern.nodes_on_host_rank.size());
                std::vector<Parfait::Point<double>> vertices(cell_length);
                for (int n = 0; n < cell_length; n++) {
                    vertices[n] = donor_mesh->node(n);
                }
                pattern.distance_to_donor_cell = best.distance;
                pattern.query_point = search_point.p;
                donor_pattern_replies[source_rank].push_back(pattern);
            }
        }

        mp.Barrier();
        mp_rootprint("\nSearching Tree Done!         \n");
        std::map<int, MessagePasser::Message> donor_replies_as_messages;

        for (auto& pair : donor_pattern_replies) {
            MessagePasser::Message m;
            int num_patterns = pair.second.size();
            m.pack(num_patterns);
            for (int i = 0; i < num_patterns; i++) {
                pair.second[i].pack(m);
            }
            donor_replies_as_messages[pair.first] = m;
        }

        return donor_replies_as_messages;
    }

    std::map<int, DonorPattern> buildDonorPatterns() {
        auto tree = createADTFromCells();
        auto e = tree->boundingExtent();
        extent_of_each_partition = mp.Gather(e);
        if (mp.Rank() == 0) {
            Parfait::ExtentWriter::write(".rank-extents", extent_of_each_partition);
        }
        std::map<int, DonorPattern> donor_patterns_here;

        mp_rootprint("Initial search\n");
        auto incoming_queries = mp.Exchange(buildSendQueries(ids_of_receptor_nodes));
        auto replies = buildReplies(tree, incoming_queries);
        incoming_queries.clear();
        auto donor_pattern_reply_incoming = mp.Exchange(replies);
        replies.clear();
        processIncomingDonorPatterns(donor_pattern_reply_incoming, donor_patterns_here);
        donor_pattern_reply_incoming.clear();

        search_policy = EXHAUSTIVE;
        std::vector<int> orphans;
        for (int n = 0; n < receptor_point_cloud->nodeCount(); n++) {
            if (donor_patterns_here.count(n) == 0) {
                orphans.push_back(n);
            }
        }
        long orphan_count = orphans.size();
        orphan_count = mp.ParallelSum(orphan_count);
        if (orphan_count == 0) {
            mp_rootprint("No orphans found, finished searching\n");
            return donor_patterns_here;
        }
        mp_rootprint("Searching for %ld orphan nodes\n", orphan_count);
        incoming_queries = mp.Exchange(buildSendQueries(orphans));
        orphans.clear();
        replies = buildReplies(tree, incoming_queries);
        tree.reset();
        incoming_queries.clear();
        donor_pattern_reply_incoming = mp.Exchange(replies);
        replies.clear();
        processIncomingDonorPatterns(donor_pattern_reply_incoming, donor_patterns_here);
        return donor_patterns_here;
    }
    void processIncomingDonorPatterns(
        std::map<int, MessagePasser::Message>& donor_pattern_reply_incoming,
        std::map<int, DonorPattern>& donor_patterns_here) const {
        for (auto& pair : donor_pattern_reply_incoming) {
            int num_patterns = -1;
            pair.second.unpack(num_patterns);
            for (int i = 0; i < num_patterns; i++) {
                DonorPattern donor_pattern;
                donor_pattern.unpack(pair.second);
                if (donor_patterns_here.count(donor_pattern.receptor_id) == 0)
                    donor_patterns_here[donor_pattern.receptor_id] = donor_pattern;
                else {
                    if (donor_pattern.distance_to_donor_cell <
                        donor_patterns_here[donor_pattern.receptor_id].distance_to_donor_cell)
                        donor_patterns_here[donor_pattern.receptor_id] = donor_pattern;
                }
            }
        }
    }

    bool isInCell(const std::vector<int>& cell, Parfait::Point<double> p) {
        switch (cell.size()) {
            case 4:
                return isIn<4>(cell, p);
            case 5:
                return isIn<5>(cell, p);
            case 6:
                return isIn<6>(cell, p);
            case 8:
                return isIn<8>(cell, p);
            default:
                throw std::logic_error("Invalid cell-type: " + std::to_string(cell.size()));
        }
    }

    template <int N>
    bool isIn(const std::vector<int>& cell, const Parfait::Point<double>& p) {
        std::array<Parfait::Point<double>, N> element;
        for (int i = 0; i < N; i++) element[i] = donor_mesh->node(cell[i]);
        return Parfait::CellContainmentChecker::isInCell<N>(element, p);
    }

    double calcDistanceToDonor(const std::vector<int>& cell, const Parfait::Point<double>& p) {
        Parfait::Point<double> center{0, 0, 0};
        for (int node : cell) {
            auto vertex = donor_mesh->node(node);
            center += vertex;
        }
        center /= double(cell.size());
        return (p - center).magnitude();
    }
};

class FromPointCloudInterpolator : public inf::InterpolationInterface {
  public:
    struct Stencil {
        std::vector<int> neighbors;
        std::vector<double> coefficients;
    };
    FromPointCloudInterpolator(MessagePasser mp,
                               std::shared_ptr<inf::MeshInterface> donor_point_cloud_in,
                               std::shared_ptr<inf::MeshInterface> receptor_point_cloud_in)
        : mp(mp),
          donor_point_cloud(donor_point_cloud_in),
          receptor_point_cloud(receptor_point_cloud_in) {
        auto gathered_points_in_gid_order =
            inf::gatherMeshPoints(*donor_point_cloud, mp.getCommunicator(), 0);
        mp.Broadcast(gathered_points_in_gid_order, 0);

        Parfait::Adt3DPoint adt(Parfait::ExtentBuilder::build(gathered_points_in_gid_order));
        for (long i = 0; i < long(gathered_points_in_gid_order.size()); i++) {
            adt.store(i, gathered_points_in_gid_order[i]);
        }

        stencils.resize(receptor_point_cloud->nodeCount());
        for (int n = 0; n < receptor_point_cloud->nodeCount(); n++) {
            Parfait::Point<double> query_point = receptor_point_cloud->node(n);

            std::vector<int> inside;
            auto extent = Parfait::Extent<double>::extentAroundPoint(query_point, 1e-3);

            while (inside.size() < 6) {
                extent.resize(1.5);
                inside = adt.retrieve(extent);
            }
            Stencil stencil;
            stencil.neighbors = inside;
            int num_neighbors = stencil.neighbors.size();
            stencil.coefficients.resize(num_neighbors);
            std::vector<Parfait::Point<double>> neighbor_points(num_neighbors);
            for (int i = 0; i < num_neighbors; i++) {
                int neighbor_id = inside[i];
                neighbor_points[i] = gathered_points_in_gid_order[neighbor_id];
            }
            calcInverseDistanceWeights(num_neighbors,
                                       (const double*)neighbor_points.data(),
                                       (const double*)&query_point,
                                       stencil.coefficients.data());
            stencils[n] = stencil;
        }
    }

    template <typename T>
    Parfait::UnitTransformer<T> getUnitTransformer(int n, const T* points, const T* query_point) {
        Parfait::Extent<T> e{{query_point}, {query_point}};
        for (int i = 0; i < n; i++) {
            auto p = &points[3 * i];
            for (int j = 0; j < 3; j++) {
                e.lo[j] = std::min(e.lo[j], p[j]);
                e.hi[j] = std::max(e.hi[j], p[j]);
            }
        }
        return Parfait::UnitTransformer<T>(e);
    }
    template <typename T>
    void calcInverseDistanceWeights(int n, const T* points, const T* query_point, T* weights) {
        T min_distance = 1.0e-14;
        auto unit_transformer = getUnitTransformer(n, points, query_point);
        Parfait::Point<T> q{query_point};
        q = unit_transformer.ToUnitSpace(q);
        T weight_sum = 0.0;
        for (int i = 0; i < n; i++) {
            Parfait::Point<T> b{&points[3 * i]};
            b = unit_transformer.ToUnitSpace(b);
            T distance = std::max(min_distance, (b - q).magnitude());
            weights[i] = 1.0 / distance;
            weight_sum += weights[i];
        }
        // normalize
        T inv_weight_sum = 1.0;
        inv_weight_sum /= weight_sum;
        for (int i = 0; i < n; i++) {
            weights[i] *= inv_weight_sum;
        }
    }

    std::shared_ptr<inf::FieldInterface> interpolate(
        std::shared_ptr<inf::FieldInterface> f) const override {
        std::string input_association = f->association();
        PARFAIT_ASSERT(f->association() == inf::FieldAttributes::Node(),
                       "FromPointCloudInterpolator can only interpolate from points");
        int block_size = f->blockSize();
        int num_receptors = receptor_point_cloud->nodeCount();
        std::vector<double> all_interpolated_columns(num_receptors * f->blockSize());
        for (int c = 0; c < block_size; c++) {
            auto f_vector = inf::FieldTools::extractColumn(*f, c);
            auto f_interpolated = interpolateFromNodeData(f_vector);

            for (int n = 0; n < num_receptors; n++) {
                all_interpolated_columns[block_size * n + c] = f_interpolated[n];
            }
        }
        return std::make_shared<inf::VectorFieldAdapter>(
            f->name(), inf::FieldAttributes::Node(), block_size, all_interpolated_columns);
    }

    std::vector<double> interpolateFromNodeData(const std::vector<double>& field) const {
        auto global_field = FieldTools::gatherNodeField(mp, *donor_point_cloud, field, 0);
        mp.Broadcast(global_field, 0);

        std::vector<double> output_field(receptor_point_cloud->nodeCount());
        for (int receptor_node_id = 0; receptor_node_id < receptor_point_cloud->nodeCount();
             receptor_node_id++) {
            double f = 0;
            auto& s = stencils[receptor_node_id];
            for (int i = 0; i < int(s.neighbors.size()); i++) {
                int neighbor = s.neighbors[i];
                double w = s.coefficients[i];
                f += w * global_field.at(neighbor);
            }
            output_field[receptor_node_id] = f;
        }

        return output_field;
    }

  public:
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> donor_point_cloud;
    std::shared_ptr<inf::MeshInterface> receptor_point_cloud;
    std::vector<int> global_node_ids_of_donor_mesh;
    std::vector<Stencil> stencils;
};
}
