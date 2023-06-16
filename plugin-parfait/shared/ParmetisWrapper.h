
// Copyright 2016 United States Government as represented by the Administrator
// of the National Aeronautics and Space Administration. No copyright is claimed
// in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform
// is licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain a
// copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#include <MessagePasser/MessagePasser.h>
#include <assert.h>
#include <parmetis.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#if PARMETIS_MAJOR_VERSION == 3
typedef idxtype metis_int;
#else
typedef idx_t metis_int;
#endif

namespace Parfait {

namespace ParmetisWrapper {

    struct ParMetisInfo {
        metis_int* vertex_weights = NULL;
        metis_int* edge_weights = NULL;
        metis_int weight_flag = 0;  // don't weight vertices or edges
        metis_int numflag = 0;      // use C indexing, not fortran
        metis_int ncon = 1;         // Number of weights.....not sure why this is 1 instead of 0...
        metis_int options[3] = {1, 0, 0};
        metis_int edgecut;
        MPI_Comm comm;
    };

    inline std::vector<metis_int> getVertexDistributionAcrossProcs(int nproc, const long* proc_node_map);

    inline std::vector<metis_int> getAdjacencyMap(const long* ia, int nnodes);

    inline std::vector<metis_int> getAdjacency(const long* ia, const long* ja, int nnodes);

    inline int getMyNumberOfNodes(int rank, const long* proc_node_map);

    inline std::vector<real_t> getTpWeights(metis_int nparts);

    inline void printParmetisInput(const std::vector<metis_int>& vtxdist,
                                   const std::vector<metis_int>& xadj,
                                   const std::vector<metis_int>& adjncy,
                                   const std::vector<real_t>& tpwgts);

    inline void partitionMesh(MessagePasser mp,
                              int rank,
                              int nproc,
                              long* proc_node_map,
                              long* ia,
                              long* ja,
                              int* part_vec,
                              int target_number_of_partitions) {
        int nnodes = getMyNumberOfNodes(rank, proc_node_map);
        PARFAIT_ASSERT(nnodes > 0,
                       "Rank " + std::to_string(mp.Rank()) + " has 0 ParMETIS graph nodes.\nThis breaks Parmetis\n");
        ParMetisInfo parMetisInfo;
        parMetisInfo.comm = mp.getCommunicator();
        for (int i = 0; i < ia[nnodes]; i++)
            if (ja[i] < 0)
                throw std::logic_error("Rank " + std::to_string(mp.Rank()) + " negative id [" + std::to_string(i) +
                                       "] of [" + std::to_string(nnodes) + "]");

        auto vtxdist = getVertexDistributionAcrossProcs(nproc, proc_node_map);
        for (int r = 0; r < nproc; r++) {
            if (vtxdist[r] == vtxdist[r + 1]) {
                PARFAIT_THROW("ParMETIS Rank " + std::to_string(r) + " is being called with no mesh");
            }
        }
        auto xadj = getAdjacencyMap(ia, nnodes);
        auto adjncy = getAdjacency(ia, ja, nnodes);
        auto tpwgts = getTpWeights(target_number_of_partitions);
        std::vector<real_t> ubvec(parMetisInfo.ncon, 1.05);
        std::vector<metis_int> part(nnodes, 0);

        metis_int target_npart = target_number_of_partitions;
        auto metis = ParMETIS_V3_PartKway(vtxdist.data(),
                                          xadj.data(),
                                          adjncy.data(),
                                          parMetisInfo.vertex_weights,
                                          parMetisInfo.edge_weights,
                                          &parMetisInfo.weight_flag,
                                          &parMetisInfo.numflag,
                                          &parMetisInfo.ncon,
                                          &target_npart,
                                          tpwgts.data(),
                                          ubvec.data(),
                                          parMetisInfo.options,
                                          &parMetisInfo.edgecut,
                                          part.data(),
                                          &parMetisInfo.comm);

        if (metis != METIS_OK) throw std::logic_error("Metis reported a failure");
        for (int i = 0; i < nnodes; i++) part_vec[i] = part[i];
    }

    std::vector<real_t> getTpWeights(metis_int nparts) {
        ParMetisInfo parMetisInfo;
        std::vector<real_t> tpwgts(parMetisInfo.ncon * nparts);
        real_t w = 1.0 / (real_t)nparts;
        for (int i = 0; i < parMetisInfo.ncon * nparts; i++) tpwgts[i] = w;
        return tpwgts;
    }

    int getMyNumberOfNodes(int rank, const long* proc_node_map) {
        return proc_node_map[rank + 1] - proc_node_map[rank];
    }

    inline std::vector<metis_int> getAdjacency(const long* ia, const long* ja, int nnodes) {
        int ia_size = nnodes + 1;
        std::vector<metis_int> adjncy(ia[ia_size - 1]);
        for (int i = 0; i < ia[ia_size - 1]; i++) adjncy[i] = ja[i];
        return adjncy;
    }

    inline std::vector<metis_int> getAdjacencyMap(const long* ia, int nnodes) {
        int ia_size = nnodes + 1;
        std::vector<metis_int> xadj(ia_size);
        for (int i = 0; i < ia_size; i++) xadj[i] = ia[i];
        return xadj;
    }

    inline std::vector<metis_int> getVertexDistributionAcrossProcs(int nproc, const long* proc_node_map) {
        std::vector<metis_int> vtxdist(nproc + 1);
        for (int i = 0; i <= nproc; i++) vtxdist[i] = proc_node_map[i];
        return vtxdist;
    }

    template <typename Graph>
    std::vector<int> generatePartVector(MessagePasser mp, const Graph& graph, int target_number_of_partitions) {
        mp_rootprint("Generating ParMETIS graph\n");
        // convert connectivity to flat arrays
        std::vector<long> ia(graph.size() + 1, 0);
        for (unsigned int i = 0; i < graph.size(); i++) {
            ia[i + 1] = ia[i] + graph[i].size();
        }
        std::vector<long> ja;
        ja.reserve(ia.back());
        for (auto& row : graph)
            for (auto nbr : row) ja.push_back(nbr);
        // map nodes to processors
        std::vector<long> procNodeMap(mp.NumberOfProcesses(), 0);
        mp.Gather((long)graph.size(), procNodeMap);
        procNodeMap.insert(procNodeMap.begin(), 0);
        for (unsigned int i = 1; i < procNodeMap.size(); i++) procNodeMap[i] += procNodeMap[i - 1];
        // allocate part vector and call 3rd party partitioner
        std::vector<int> part(graph.size(), 0);
        partitionMesh(mp,
                      mp.Rank(),
                      mp.NumberOfProcesses(),
                      procNodeMap.data(),
                      ia.data(),
                      ja.data(),
                      part.data(),
                      target_number_of_partitions);
        return part;
    }

    template <typename Graph>
    std::vector<int> generatePartVector(MessagePasser mp, const Graph& graph) {
        return generatePartVector(mp, graph, mp.NumberOfProcesses());
    }

}  // namespace ParmetisWrapper
}  // namespace Parfait