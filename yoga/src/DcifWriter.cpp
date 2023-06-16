#include <MessagePasser/MessagePasser.h>
#include <parfait/ByteSwap.h>
#include <cstdio>
#include "DcifWriter.h"
#include "YogaMesh.h"

namespace YOGA {
    struct DcifHeader{
        long nnodes;
        long nreceptors;
        long ndonors;
    };

long countOwnedNodes(MessagePasser mp, const YogaMesh& mesh){
    long n = 0;
    for(int i=0;i<mesh.nodeCount();i++)
        if(mp.Rank() == mesh.nodeOwner(i))
            n++;
    return n;
}

DcifHeader getDcifHeader(MessagePasser mp, const YogaMesh& mesh, const DomainConnectivityInfo<double>& dci){
    DcifHeader h;
    h.nnodes = countOwnedNodes(mp, mesh);
    h.nnodes = mp.ParallelSum(h.nnodes,0);
    h.nreceptors = dci.receptorGids.size();
    h.nreceptors = mp.ParallelSum(h.nreceptors,0);
    h.ndonors = 0;
    for(auto& x:dci.donorGids) h.ndonors += x.size();
    h.ndonors = mp.ParallelSum(h.ndonors,0);
    return h;
}

    void DcifWriter::exportDcif(std::string name, YogaMesh& mesh, DomainConnectivityInfo<double>& dci) {

        auto dcif_header = getDcifHeader(mp,mesh,dci);
        int ncomponents = countComponentGridIds(mesh);

        auto globalFringeIds = gatherGlobalFringeIds(dci);
        auto globalDonorCounts = gatherDonorCounts(dci);
        auto globalDonorIds = gatherDonorIds(dci);
        auto globalDonorWeights = gatherWeights(dci);
        auto globalIblank = gatherNodeStatuses(dci);
        auto globalNodeIds = gatherGlobalNodeIds(mesh);
        auto globalComponentIds = gatherComponentIds(mesh);

        if(mp.Rank() == 0) {
            FILE *f = NULL;
            openFileOnRoot(f, name);
            writeHeaderInfo(f, dcif_header.nnodes, dcif_header.nreceptors, dcif_header.ndonors, ncomponents);
            writeFringes(f, globalFringeIds);
            writeDonorCounts(f, globalDonorCounts);
            writeDonorIds(f, globalDonorIds);
            writeDonorWeights(f, globalDonorWeights);
            writeIblank(f, globalIblank, globalNodeIds);
            writeFooterInfo(f, globalComponentIds,globalNodeIds);
            closeFileOnRoot(f);
        }
    }
    std::vector<std::vector<int>> DcifWriter::gatherComponentIds(const YogaMesh& mesh) {
        std::vector<int> localComponentIds;
        for(int i=0;i<mesh.nodeCount();++i)
            localComponentIds.push_back(mesh.getAssociatedComponentId(i));
        std::vector<std::vector<int>> globalComponentIds;
        mp.Gather(localComponentIds,globalComponentIds,0);
        return globalComponentIds;
    }
    std::vector<std::vector<long>> DcifWriter::gatherGlobalNodeIds(const YogaMesh& mesh) {
        std::vector<long> globalIdsForLocalNodes;
        for(int i=0;i<mesh.nodeCount();++i)
            if(mp.Rank() == mesh.nodeOwner(i))
                globalIdsForLocalNodes.push_back(mesh.globalNodeId(i));
        std::vector<std::vector<long>> globalNodeIds;
        mp.Gather(globalIdsForLocalNodes,globalNodeIds,0);
        return globalNodeIds;
    }
    std::vector<std::vector<int8_t>> DcifWriter::gatherNodeStatuses(DomainConnectivityInfo<double>& dci) {
        std::vector<int8_t> localIblank;
        for(auto status:dci.nodeStatuses)
            localIblank.push_back(convertStatusToIblank(status));
        std::vector<std::vector<int8_t>> globalIblank;
        mp.Gather(localIblank,globalIblank,0);
        return globalIblank;
    }
    std::vector<std::vector<double>> DcifWriter::gatherWeights(DomainConnectivityInfo<double>& dci) {
        std::vector<double> localDonorWeights;
        for(auto& donorWeightsForReceptor:dci.weights)
            for(double w:donorWeightsForReceptor)
                localDonorWeights.push_back(w);
        std::vector<std::vector<double>> globalDonorWeights;
        mp.Gather(localDonorWeights,globalDonorWeights,0);
        return globalDonorWeights;
    }
    std::vector<std::vector<long>> DcifWriter::gatherDonorIds(DomainConnectivityInfo<double>& dci) {
        std::vector<long> localDonorIds;
        for(auto& donorsForReceptor:dci.donorGids)
            for(long id:donorsForReceptor)
                localDonorIds.push_back(id);
        std::vector<std::vector<long>> globalDonorIds;
        mp.Gather(localDonorIds,globalDonorIds,0);
        return globalDonorIds;
    }
    std::vector<std::vector<int8_t>> DcifWriter::gatherDonorCounts(DomainConnectivityInfo<double>& dci) {
        std::vector<int8_t> localDonorCounts;
        for(auto& donorsForReceptor:dci.donorGids)
            localDonorCounts.push_back(donorsForReceptor.size());
        std::vector<std::vector<int8_t>> globalDonorCounts;
        mp.Gather(localDonorCounts,globalDonorCounts,0);
        return globalDonorCounts;
    }
    std::vector<std::vector<long>> DcifWriter::gatherGlobalFringeIds(const DomainConnectivityInfo<double>& dci) {
        std::vector<std::vector<long>> globalFringeIds;
        mp.Gather(dci.receptorGids,globalFringeIds,0);
        return globalFringeIds;
    }
    int DcifWriter::countComponentGridIds(const YogaMesh& mesh) {
        std::set<int> componentIds;
        for(int i=0;i<mesh.nodeCount();++i)
            componentIds.insert(mesh.getAssociatedComponentId(i));
        std::vector<int> idVec(componentIds.begin(),componentIds.end());
        std::vector<std::vector<int>> allVectors;
        mp.Gather(idVec, allVectors,0);
        for(auto& vec:allVectors)
            for(int id:vec)
                componentIds.insert(id);
        int ncomponents = componentIds.size();
        return ncomponents;
    }

    inline void DcifWriter::openFileOnRoot(FILE *&f, std::string name) {
        if (mp.Rank() == 0)
            f = fopen(name.c_str(), "w");
    }

    inline void DcifWriter::closeFileOnRoot(FILE *&f) {
        if (mp.Rank() == 0)
            fclose(f);
    }

    inline void DcifWriter::writeHeaderInfo(FILE *f, long nnodes, long nreceptors, long ndonors, int ncomponents) {
        printf("=======================================\n");
        printf("========= writing dcif file ===========\n");
        printf("=======================================\n");
        printf("nnodes       %li\n", nnodes);
        printf("nfringes     %li\n", nreceptors);
        printf("ndonors      %li\n", ndonors);
        printf("ngrids       %i\n", ncomponents);
        // swap bytes if needed
        fwrite(&nnodes, 8, 1, f);
        fwrite(&nreceptors, 8, 1, f);
        fwrite(&ndonors, 8, 1, f);
        fwrite(&ncomponents, 4, 1, f);
    }

    inline void DcifWriter::writeFringes(FILE *f, std::vector<std::vector<long>>& fringeIds) {
        for(auto& fringes: fringeIds) {
            convertToFortranIndexing(fringes);
            fwrite(fringes.data(), 8, fringes.size(), f);
        }
    }

    inline void DcifWriter::writeDonorCounts(FILE *f, std::vector<std::vector<int8_t>>& globalDonorCounts) {
        for(auto& donorCountsFromProc:globalDonorCounts)
            fwrite(donorCountsFromProc.data(), 1, donorCountsFromProc.size(), f);
    }

    inline void DcifWriter::writeDonorIds(FILE *f, std::vector<std::vector<long>>& globalDonorIds) {
        for(auto& donorIdsFromProc:globalDonorIds) {
            convertToFortranIndexing(donorIdsFromProc);
            fwrite(donorIdsFromProc.data(), 8, donorIdsFromProc.size(), f);
        }
    }

    inline void DcifWriter::writeDonorWeights(FILE *f, std::vector<std::vector<double>>& globalDonorWeights) {
        for(auto& weightsFromProc:globalDonorWeights)
            fwrite(weightsFromProc.data(), 8, weightsFromProc.size(), f);
    }

    inline void DcifWriter::writeIblank(FILE *f, std::vector<std::vector<int8_t>>& globalIblank,
                                        std::vector<std::vector<long>>& globalNodeIds) {
        long n_receptor = 0;
        long n_in = 0;
        long n_out = 0;
        long n_orphan = 0;
        long iblankSize = 0;
        for(auto& iblank:globalIblank)
            iblankSize += iblank.size();
        std::vector<int8_t> sortedIblank(iblankSize,-9);
        for(int proc=0;proc<mp.NumberOfProcesses();++proc){
            auto& iblank = globalIblank[proc];
            auto& ids = globalNodeIds[proc];
            for(size_t i=0;i<iblank.size();++i)
                sortedIblank[ids[i]] = iblank[i];
        }

        for(auto s:sortedIblank) {
            if (-1 == s)
                n_receptor++;
            else if (-2 == s)
                n_orphan++;
            else if (0 == s)
                n_out++;
            else if (1 == s)
                n_in++;
            else
                throw std::logic_error("Unrecognized status: " + std::to_string(s));
        }
        printf("     # receptor: %li\n",n_receptor);
        printf("     # in:       %li\n",n_in);
        printf("     # out:      %li\n",n_out);
        printf("     # orphan:   %li\n",n_orphan);
        printf("      total:     %li\n",n_receptor+n_in+n_out+n_orphan);
        fwrite(sortedIblank.data(), 1, sortedIblank.size(), f);
    }


    inline void DcifWriter::writeFooterInfo(FILE *f, std::vector<std::vector<int>>& globalComponentIds,
                                            std::vector<std::vector<long>>& globalNodeIds) {
        // map node ids onto the components they belong to
        std::map<int,std::vector<long>> meshIdToGlobalNodes;
        for(int proc=0;proc<mp.NumberOfProcesses();++proc){
            for(size_t i=0;i<globalNodeIds[proc].size();++i){
                int componentId = globalComponentIds[proc][i];
                long nodeId = globalNodeIds[proc][i];
                if(meshIdToGlobalNodes.find(componentId) == meshIdToGlobalNodes.end())
                    meshIdToGlobalNodes[componentId] = {nodeId};
                else
                    meshIdToGlobalNodes[componentId].push_back(nodeId);
            }
        }

        int max_component_id = 0;
        for(auto& pair:meshIdToGlobalNodes)
            max_component_id = std::max(max_component_id,pair.first);

        auto convert_component_id_to_fun3d_imesh = [=](int component_id){
            if(component_id == max_component_id)
                return 0;  // assume last grid in composite is stationary
            else
                return component_id + 1; // assume all other grids are dynamic and start at 1
        };

        // write out the footer
        long offset = 0;
        for(auto& pair:meshIdToGlobalNodes){
            long start = offset;
            offset +=pair.second.size();
            long stop = offset;
            int component_id = pair.first;
            int imesh = convert_component_id_to_fun3d_imesh(component_id);
            fwrite(&start, 8, 1, f);
            fwrite(&stop, 8, 1, f);
            fwrite(&imesh, 4, 1, f);
        }
    }

    int DcifWriter::convertStatusToIblank(NodeStatus s) {
        switch (s){
            case NodeStatus::OutNode:
                return 0;
            case NodeStatus::InNode:
                return 1;
            case NodeStatus::Orphan:
                return -2;
            case NodeStatus::FringeNode:
                return -1;
            default:
                return 0;
        }
    }
    DcifWriter::DcifWriter(MessagePasser mp_in) :mp(mp_in) {}
    }
