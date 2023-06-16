#pragma once
#include <string>
#include "YogaStatuses.h"
#include "DomainConnectivityInfo.h"
#include "YogaMesh.h"

namespace YOGA {
    class DcifWriter {
      public:
        DcifWriter(MessagePasser mp);

        void exportDcif(std::string name, YogaMesh& mesh, DomainConnectivityInfo<double>& dci);

        void openFileOnRoot(FILE *&f, std::string name);
        void closeFileOnRoot(FILE *&f);

        void writeHeaderInfo(FILE *f, long nnodes, long nreceptors, long ndonors, int ncomponents);
        void writeFringes(FILE *f, std::vector<std::vector<long>>& fringeIds);
        void writeDonorCounts(FILE *f, std::vector<std::vector<int8_t>>& globalDonorCounts);
        void writeDonorIds(FILE *f, std::vector<std::vector<long>>& globalDonorIds);
        void writeDonorWeights(FILE *f, std::vector<std::vector<double>>& globalDonorIds);
        void writeIblank(FILE *f, std::vector<std::vector<int8_t>>& globalIblank,
                         std::vector<std::vector<long>>& globalNodeIds);
        void writeFooterInfo(FILE *f, std::vector<std::vector<int>>& globalComponentIds,
                             std::vector<std::vector<long>>& globalNodeIds);

      private:
        MessagePasser mp;
        template<typename T>
        void convertToFortranIndexing(T &ids) {
          for (auto &id:ids) id++;
        }

        long countOwnedNodes(const YogaMesh& mesh);

        int convertStatusToIblank(NodeStatus s);

        int countComponentGridIds(const YogaMesh& mesh);
        std::vector<std::vector<long>> gatherGlobalFringeIds(const DomainConnectivityInfo<double>& dci);
        std::vector<std::vector<int8_t>> gatherDonorCounts(DomainConnectivityInfo<double>& dci);
        std::vector<std::vector<long>> gatherDonorIds(DomainConnectivityInfo<double>& dci);
        std::vector<std::vector<double>> gatherWeights(DomainConnectivityInfo<double>& dci);
        std::vector<std::vector<int8_t>> gatherNodeStatuses(DomainConnectivityInfo<double>& dci);
        std::vector<std::vector<long>> gatherGlobalNodeIds(const YogaMesh& mesh);
        std::vector<std::vector<int>> gatherComponentIds(const YogaMesh& mesh);
    };
}