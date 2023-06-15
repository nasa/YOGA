#pragma once
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include <parfait/ByteSwap.h>
#include <DcifDistributor.h>
#include <cstdlib>

namespace YOGA{

class SuggarDciReader{
  public:
    SuggarDciReader(std::string filename);
    long globalNodeCount() const{return node_statuses.size();}
    long receptorCount() const{return receptors.size();}
    long donorCount() const{return ndonors;}
    int gridCount() const{return ngrids;}

    int nodesInGrid(int i);
    std::vector<Dcif::Receptor> getReceptors(long start,long end){
        return std::vector<Dcif::Receptor>(receptors.begin()+start,receptors.begin()+end);
    }
    std::vector<int> getNodeStatuses(){return node_statuses;}
    std::vector<long> getGridOffsets(){return grid_offsets;}
  private:
    int ngrids = 0;
    long ndonors = 0;
    std::vector<long> grid_offsets;
    std::vector<Dcif::Receptor> receptors;
    std::vector<int> node_statuses;

    void readBlockOfReceptors(int receptor_grid_id,const std::vector<char>& buffer, std::string& key, long& pos);
    long getGlobalId(int local_id, int grid_id);
    void readReceptor(int receptor_grid_id, const std::vector<char>& buffer, long& pos);
    void readBlockOfIblank(int receptor_grid_id, std::vector<char> buffer, std::string key, long& pos);
};
}