#pragma once
#include <string>
#include <stdio.h>
#include <vector>
#include <DcifChecker.h>

namespace YOGA {
  class DcifReader {
  public:
      DcifReader(std::string filename);
      long globalNodeCount(){return nnodes;}
      long receptorCount(){return nfringes;}
      long donorCount(){return ndonors;}
      int gridCount(){return ngrids;}
      std::vector<int64_t> getFringeIds() { return fringe_ids; }
      std::vector<int8_t> getIblank() { return iblank; }
      std::vector<int8_t> getDonorCounts() { return donor_counts; }
      std::vector<int64_t> getDonorIds() { return donor_ids; }
      std::vector<double> getDonorWeights() { return donor_weights; }
      int64_t nodeCountForComponentGrid(int component){
          return 1+component_grid_end_gid[component]-component_grid_start_gid[component];
      }
      int32_t imeshForComponentGrid(int component){return component_grid_imesh[component];}
  private:
      int64_t nnodes;
      int64_t nfringes;
      int64_t ndonors;
      int32_t ngrids;
      std::vector<int64_t> fringe_ids;
      std::vector<int8_t> donor_counts;
      std::vector<int64_t> donor_ids;
      std::vector<double> donor_weights;
      std::vector<int8_t> iblank;
      std::vector<int64_t> component_grid_start_gid;
      std::vector<int64_t> component_grid_end_gid;
      std::vector<int32_t> component_grid_imesh;

      void readFile(FILE *f);
      void convertBackFromFortranIndexing();
      bool anyCountsAreNegativeOrRidiculouslyBig();
      void printDcifHeader() const;
      template<typename T>
      void swapBytesInVector(std::vector<T>& v) {
          if(8 == sizeof(T))
            for(auto& x:v)
                bswap_64(&x);
          else if(4 == sizeof(T))
              for(auto& x:v)
                  bswap_32(&x);
      }
  };
}