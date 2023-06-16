
namespace YOGA {

template<typename T>
std::map<int, std::vector<ReceptorUpdateRequest<T>>> mapPointsToRanks(
    const std::map<int, OversetData::DonorCell>& receptors, const YogaMesh& mesh) {
    std::map<int, std::vector<ReceptorUpdateRequest<T>>> pointsForRank;

    for (auto& r : receptors) {
        int node_id = r.first;
        auto donor = r.second;
        int owner = donor.owningRank;
        int cellId = donor.id;
        pointsForRank[owner].push_back(ReceptorUpdateRequest<T>(cellId, node_id, mesh.getNode<T>(node_id)));
    }
    return pointsForRank;
}

template<typename T>
std::map<int, std::vector<InverseReceptor<T>>> generateInverseReceptors(
    MessagePasser mp, const std::map<int, OversetData::DonorCell>& receptors, const YogaMesh& mesh) {
    Tracer::begin("map points to ranks");
    auto pointsForRank = mapPointsToRanks<T>(receptors, mesh);
    Tracer::end("map points to ranks");
    Tracer::begin("exchange");
    auto recvd_receptors = mp.Exchange(pointsForRank);
    Tracer::end("exchange");
    std::map<int, std::vector<InverseReceptor<T>>> inverse_receptors;

    Tracer::begin("fill inverse receptors");
    for (auto& pair : recvd_receptors) {
        int rank = pair.first;
        for (auto& r : pair.second) {
            InverseReceptor<T> ir(r.containingCellId, r.localNodeId, r.p);
            inverse_receptors[rank].push_back(ir);
        }
    }
    Tracer::end("fill inverse receptors");
    return inverse_receptors;
}

void exchangeReceptorUpdates(const MessagePasser& mp,
                             int nvar,
                             const std::map<int, OversetData::DonorCell>& receptors,
                             const std::map<int, std::vector<double>>& receptor_update_solutions,
                             std::vector<double>& solution_at_nodes) {
    Tracer::begin("exchange updates");
    std::map<int, MessagePasser::MessageStatus> mpi_sends, mpi_waits;
    Tracer::begin("allocate space for recvs");
    std::map<int, std::vector<int>> ids_expected_from_rank;
    for (auto& pair : receptors) {
        int my_local_node_id = pair.first;
        auto& donor_cell = pair.second;
        int rank = donor_cell.owningRank;
        ids_expected_from_rank[rank].push_back(my_local_node_id);
    }
    std::map<int, std::vector<double>> my_update_solutions;
    for (auto& pair : ids_expected_from_rank) {
        int rank = pair.first;
        int count = pair.second.size();
        my_update_solutions[rank].resize(nvar * count);
    }
    Tracer::end("allocate space for recvs");
    // post recvs
    Tracer::begin("post recvs");
    for (auto& pair : my_update_solutions) {
        int rank = pair.first;
        int count = pair.second.size();
        mpi_waits[rank] = mp.NonBlockingRecv(pair.second, count, rank);
    }
    Tracer::end("post recvs");
    // post sends
    Tracer::begin("post sends");
    for (auto& pair : receptor_update_solutions) {
        int target_rank = pair.first;
        int count = pair.second.size();
        mpi_sends[target_rank] = mp.NonBlockingSend(pair.second, count, target_rank);
    }
    Tracer::end("post sends");
    Tracer::end("exchange updates");
    Tracer::begin("unpack updates");
    for (auto& pair : ids_expected_from_rank) {
        int rank = pair.first;
        int n = ids_expected_from_rank[rank].size();
        mpi_waits.at(rank).wait();
        for (int i = 0; i < n; i++) {
            int node_id = ids_expected_from_rank[rank][i];
            double* q = &my_update_solutions[rank][nvar * i];
            for (int j = 0; j < nvar; j++) solution_at_nodes[nvar * node_id + j] = q[j];
        }
    }
    Tracer::end("unpack updates");
    Tracer::begin("wait for sends to complete");
    for (auto& s : mpi_sends) s.second.wait();
    Tracer::end("wait for sends to complete");
}

template<typename T>
void getUpdatedSolutionsFromInterpolator(MessagePasser mp,
                                         const std::map<int, OversetData::DonorCell>& receptors,
                                         const std::map<int, std::vector<InverseReceptor<T>>> inverse_receptor_map,
                                         int nvar,
                                         const YOGA::Interpolator<T>& interpolator,
                                         const std::function<void(int, double*)>& getSolutionAtNode,
                                         std::vector<double>& solution_at_nodes) {
    Tracer::begin("Get interpolated solutions from solver");
    std::map<int, std::vector<double>> receptor_update_solutions;
    for (auto& pair : inverse_receptor_map) {
        int rank = pair.first;
        auto& inverse_receptors = pair.second;
        int n = inverse_receptors.size();
        auto& update_solutions = receptor_update_solutions[rank];
        update_solutions.resize(nvar * n);
        for (int i = 0; i < n; i++) {
            interpolator.interpolate(rank, i, getSolutionAtNode, &update_solutions[nvar * i]);
        }
    }
    Tracer::end("Get interpolated solutions from solver");
    exchangeReceptorUpdates(mp, nvar, receptors, receptor_update_solutions, solution_at_nodes);
}

template<typename T>
void getUpdatedSolutionsFromSolver(MessagePasser mp,
                                   const YogaMesh& mesh,
                                   const std::map<int, OversetData::DonorCell>& receptors,
                                   const std::map<int, std::vector<InverseReceptor<T>>> inverse_receptor_map,
                                   int nvar,
                                   std::function<void(int, double, double, double, double*)> getter,
                                   std::vector<double>& solution_at_nodes) {
    Tracer::begin("Get interpolated solutions from solver");
    std::map<int, std::vector<double>> receptor_update_solutions;
    for (auto& pair : inverse_receptor_map) {
        int rank = pair.first;
        auto& inverse_receptors = pair.second;
        int n = inverse_receptors.size();
        auto& update_solutions = receptor_update_solutions[rank];
        update_solutions.resize(nvar * n);
        for (int i = 0; i < n; i++) {
            auto& inverse_receptor = inverse_receptors[i];
            auto& xyz = inverse_receptor.p;
            int original_mesh_cell_id = mesh.getCellIdFromOriginalMesh(inverse_receptor.donor_cell_id);
            getter(original_mesh_cell_id, xyz[0], xyz[1], xyz[2], &update_solutions[nvar * i]);
        }
    }
    Tracer::end("Get interpolated solutions from solver");
    exchangeReceptorUpdates(mp, nvar, receptors, receptor_update_solutions, solution_at_nodes);
}

}
