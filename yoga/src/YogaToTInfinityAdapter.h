#pragma once
#include <t-infinity/MeshInterface.h>
#include "YogaMesh.h"

namespace YOGA{
class YogaToTInfinityAdapter : public inf::MeshInterface{
  public:
    YogaToTInfinityAdapter(const YogaMesh& m,const std::map<long,int>& global_to_local,MessagePasser mp);
    virtual int nodeCount() const override;
    virtual int cellCount() const override;
    virtual int partitionId() const override;
    virtual int cellCount(MeshInterface::CellType cell_type) const override;
    virtual void nodeCoordinate(int node_id, double* coord) const override;
    virtual MeshInterface::CellType cellType(int cell_id) const override;
    virtual void cell(int cell_id, int* cell) const override;
    virtual long globalNodeId(int node_id) const override;
    virtual long globalCellId(int cell_id) const override;
    virtual int cellTag(int cell_id) const override;
    virtual int nodeOwner(int node_id) const override;
    virtual int cellOwner(int cell_id) const override;
    std::string tagName(int t) const override;
  private:
    const YogaMesh& m;
    const std::map<long,int>& g2l;
    const int rank;
    std::vector<long> global_cell_ids;
    void cell(int cell_id,std::vector<int>& cell) const;
    bool isVolumeCell(int cell_id) const;
    bool isCellMine(const std::vector<int>& cell_nodes);
    long calcGlobalCellIdOffset(MessagePasser mp);
    std::vector<long> generateGlobalCellIds(MessagePasser passer);
    void setGlobalIdsForOwnedCells(const MessagePasser& mp,
                                   long gid_offset_for_rank,
                                   std::vector<long>& cell_gids) const;
    void setGlobalIdsForGhostCells(MessagePasser mp,
                                   const std::vector<std::vector<int>>& n2c,
                                   inf::MeshInterface::CellType,
                                   std::vector<long>& cell_gids);
    bool isSameCell(const std::vector<long>& a,const std::vector<long>& b);
    void setGhostGidsFromReplies(std::vector<long>& cell_gids,
                                 const std::map<int,std::vector<int>>& local_cell_ids_of_requests,
                                 const std::map<int, std::vector<long>>& reply_gids_from_ranks) const;
    std::map<int,std::vector<long>> buildReplyGIDs(const std::map<int,std::vector<long>>& cells_from_ranks,
                                                   const std::vector<std::vector<int>>& n2c,
                                                   const std::vector<long>& cell_gids,
                                                   int cell_length);
    void setCellsForRanksAndLocalIdsOfRequestedCells(
        std::map<int,std::vector<long>>& cells_for_ranks,
        std::map<int,std::vector<int>>& local_cell_ids_of_requests,
        inf::MeshInterface::CellType cell_type);
};

}