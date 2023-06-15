#include "YogaToTInfinityAdapter.h"
#include <t-infinity/MeshConnectivity.h>
#include <algorithm>

namespace YOGA{
using namespace inf;

int getLowestGlobalNodeId(const std::vector<long>& gids) {
    long lowest = gids.front();
    for(long gid : gids) lowest = std::min(lowest, gid);
    return lowest;
}

long YogaToTInfinityAdapter::calcGlobalCellIdOffset(MessagePasser mp){
    long cells_i_own = 0;
    for(int i=0;i<cellCount();i++)
        if(rank == cellOwner(i))
            ++cells_i_own;

    std::vector<long> cells_per_rank(mp.NumberOfProcesses(),0);
    cells_per_rank[rank] = cells_i_own;
    cells_per_rank = mp.ElementalMax(cells_per_rank);


    long my_offset = 0;
    for(int i=0;i<rank;i++)
        my_offset += cells_per_rank[i];
    return my_offset;
}

YogaToTInfinityAdapter::YogaToTInfinityAdapter(const YogaMesh& m,const std::map<long,int>& global_to_local,
                                               MessagePasser mp)
    :m(m),
      g2l(global_to_local),
      rank(mp.Rank()),
      global_cell_ids(generateGlobalCellIds(mp))
{
}


int YogaToTInfinityAdapter::nodeCount() const { return m.nodeCount(); }
int YogaToTInfinityAdapter::cellCount() const { return m.numberOfCells()+m.numberOfBoundaryFaces(); }
int YogaToTInfinityAdapter::partitionId() const { return rank; }

int countFacesOfSize(const YogaMesh& m,int size){
    int n = 0;
    for(int i=0;i<m.numberOfBoundaryFaces();i++)
        if(size == m.numberOfNodesInBoundaryFace(i))
            n++;
    return n;
}
int countTriangles(const YogaMesh& m){return countFacesOfSize(m,3);}
int countQuads(const YogaMesh& m){return countFacesOfSize(m,4);}

int YogaToTInfinityAdapter::cellCount(inf::MeshInterface::CellType cell_type) const {
    if(CellType::TETRA_4 == cell_type)
        return m.cellCount(YogaMesh::TET);
    if(CellType::PYRA_5 == cell_type)
        return m.cellCount(YogaMesh::PYRAMID);
    if(CellType::PENTA_6 == cell_type)
        return m.cellCount(YogaMesh::PRISM);
    if(CellType::HEXA_8 == cell_type)
        return m.cellCount(YogaMesh::HEX);
    if(CellType::TRI_3 == cell_type)
        return countTriangles(m);
    if(CellType::QUAD_4 == cell_type)
        return countQuads(m);
    return 0;
}

void YogaToTInfinityAdapter::nodeCoordinate(int node_id, double* coord) const {
    auto p = m.getNode<double>(node_id);
    std::copy(p.data(),p.data()+3,coord);
}

inf::MeshInterface::CellType YogaToTInfinityAdapter::cellType(int cell_id) const {
    if(isVolumeCell(cell_id)){
        int n = m.numberOfNodesInCell(cell_id);
        if(4 == n)
            return CellType::TETRA_4;
        else if(5 == n)
            return CellType::PYRA_5;
        else if(6 == n)
            return CellType::PENTA_6;
        else if(8 == n)
            return CellType::HEXA_8;
        else
            throw std::logic_error("YogaToTInfinityAdapter: Unkown cell-type for cell of length: "+std::to_string(n));
    } else{
        int face_id = cell_id - m.numberOfCells();
        int n = m.numberOfNodesInBoundaryFace(face_id);
        if(3 == n)
            return CellType::TRI_3;
        else if(4 == n)
            return CellType::QUAD_4;
        else
            throw std::logic_error("YogaToTInfinityAdapter: Unkown cell-type for face of length: "+std::to_string(n));
    }
}
bool YogaToTInfinityAdapter::isVolumeCell(int cell_id) const { return cell_id < m.numberOfCells(); }

void YogaToTInfinityAdapter::cell(int cell_id, int* cell) const {
   if(isVolumeCell(cell_id)){
       m.getNodesInCell(cell_id,cell);
   }
   else{
       int face_id = cell_id - m.numberOfCells();
       auto face = m.getNodesInBoundaryFace(face_id);
       std::copy(face.begin(),face.end(),cell);
   }
}

long YogaToTInfinityAdapter::globalNodeId(int node_id) const {
    return m.globalNodeId(node_id);
}

long YogaToTInfinityAdapter::globalCellId(int cell_id) const {
    return global_cell_ids[cell_id];
}

int YogaToTInfinityAdapter::cellTag(int cell_id) const {
    if(isVolumeCell(cell_id))
        return 0;
    int face_id = cell_id - m.numberOfCells();
    return m.getBoundaryCondition(face_id);
}

int YogaToTInfinityAdapter::nodeOwner(int node_id) const {
    return m.nodeOwner(node_id);
}

int YogaToTInfinityAdapter::cellOwner(int cell_id) const {
    std::vector<int> cell_nodes;
    cell(cell_id,cell_nodes);
    std::vector<long> gids;
    for(int id:cell_nodes)
        gids.push_back(globalNodeId(id));
    long gid = getLowestGlobalNodeId(gids);
    return nodeOwner(g2l.at(gid));
}

std::string YogaToTInfinityAdapter::tagName(int t) const { return MeshInterface::tagName(t); }
std::vector<long> YogaToTInfinityAdapter::generateGlobalCellIds(MessagePasser mp) {
    long gid_offset_for_rank = calcGlobalCellIdOffset(mp);
    std::vector<long> cell_gids(cellCount(),-1);
    setGlobalIdsForOwnedCells(mp, gid_offset_for_rank, cell_gids);
    auto original_ghost_count = std::count(cell_gids.begin(), cell_gids.end(),-1);
    auto n2c = inf::NodeToCell::build(*this);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::TRI_3,cell_gids);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::QUAD_4,cell_gids);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::TETRA_4,cell_gids);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::PYRA_5,cell_gids);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::PENTA_6,cell_gids);
    setGlobalIdsForGhostCells(mp,n2c,inf::MeshInterface::HEXA_8,cell_gids);

    auto unset = std::count(cell_gids.begin(),cell_gids.end(),-1);
    if(unset > 0) {
        std::string msg = "Not all ghost-cell global ids set: ";
        msg.append(std::to_string(unset)+" unset out of initial "+std::to_string(original_ghost_count));
        throw std::logic_error(msg);
    }
    return cell_gids;
}

void YogaToTInfinityAdapter::setGlobalIdsForOwnedCells(const MessagePasser& mp,
                                                       long gid_offset_for_rank,
                                                       std::vector<long>& cell_gids) const {
    int n = 0;
    for(int i=0;i<cellCount();i++){
        if(mp.Rank() == cellOwner(i)) {
            cell_gids[i] = n + gid_offset_for_rank;
            ++n;
        }
    }
}

void YogaToTInfinityAdapter::setGlobalIdsForGhostCells(MessagePasser mp,
                                                       const std::vector<std::vector<int>>& n2c,
                                                       inf::MeshInterface::CellType cell_type,
                                                       std::vector<long>& cell_gids) {
    std::map<int,std::vector<long>> cells_for_ranks;
    std::map<int,std::vector<int>> local_cell_ids_of_requests;
    setCellsForRanksAndLocalIdsOfRequestedCells(cells_for_ranks,local_cell_ids_of_requests,cell_type);

    auto cells_from_ranks = mp.Exchange(cells_for_ranks);

    int cell_length = cellTypeLength(cell_type);
    auto reply_gids_for_ranks = buildReplyGIDs(cells_from_ranks,n2c,cell_gids,cell_length);
    auto reply_gids_from_ranks = mp.Exchange(reply_gids_for_ranks);

    setGhostGidsFromReplies(cell_gids, local_cell_ids_of_requests, reply_gids_from_ranks);

}

void YogaToTInfinityAdapter::setCellsForRanksAndLocalIdsOfRequestedCells(
    std::map<int,std::vector<long>>& cells_for_ranks,
    std::map<int,std::vector<int>>& local_cell_ids_of_requests,
    inf::MeshInterface::CellType cell_type){
    for(int i=0;i<cellCount();i++){
        if(cell_type == cellType(i)){
            int ghost_cell_owner = cellOwner(i);
            if(rank != ghost_cell_owner) {
                std::vector<int> ghost_cell;
                cell(i, ghost_cell);
                local_cell_ids_of_requests[ghost_cell_owner].push_back(i);
                for (int id : ghost_cell) cells_for_ranks[ghost_cell_owner].push_back(globalNodeId(id));
            }
        }
    }
}

std::map<int,std::vector<long>> YogaToTInfinityAdapter::buildReplyGIDs(const std::map<int,std::vector<long>>& cells_from_ranks,
                                                                        const std::vector<std::vector<int>>& n2c,
                                                                        const std::vector<long>& cell_gids,
                                                                        int cell_length){
    std::map<int,std::vector<long>> reply_gids_for_ranks;
    for(auto pair:cells_from_ranks){
        int requesting_rank = pair.first;
        auto& requested_cells = pair.second;
        int ncells_requested = requested_cells.size() / cell_length;
        for(int i=0;i<ncells_requested;i++){
            std::vector<long> requested_cell(cell_length);
            auto begin = &requested_cells[cell_length*i];
            auto end = begin + cell_length;
            std::copy(begin,end,requested_cell.begin());
            std::set<int> nbr_set;
            for(long gid:requested_cell){
                int local_id = g2l.at(gid);
                auto& nbrs = n2c.at(local_id);
                std::copy(nbrs.begin(),nbrs.end(),std::inserter(nbr_set, nbr_set.begin()));
            }
            long target_cell_gid = -1;
            for(int nbr: nbr_set){
                std::vector<int> cell_nodes;
                cell(nbr,cell_nodes);
                std::vector<long> cell_nodes_gids;
                for(int id:cell_nodes)
                    cell_nodes_gids.push_back(globalNodeId(id));
                if(isSameCell(requested_cell,cell_nodes_gids)){
                    if(rank != cellOwner(nbr))
                        throw std::logic_error("I have the cell resident, but I don't own it");
                    target_cell_gid = cell_gids[nbr];
                    break;
                }
            }
            if(target_cell_gid == -1) {
                std::string msg = "Could not find global id for requested cell:";
                for(long gid:requested_cell)
                    msg.append(" "+std::to_string(gid));
                throw std::logic_error(msg);
            }
            reply_gids_for_ranks[requesting_rank].push_back(target_cell_gid);
        }
    }
    return reply_gids_for_ranks;
}

void YogaToTInfinityAdapter::setGhostGidsFromReplies(std::vector<long>& cell_gids,
                                                     const std::map<int, std::vector<int>>& local_cell_ids_of_requests,
                                                     const std::map<int, std::vector<long>>& reply_gids_from_ranks) const {
    for(auto& pair:reply_gids_from_ranks){
        int from_rank = pair.first;
        auto& gids = pair.second;
        auto& requested_local_ids = local_cell_ids_of_requests.at(from_rank);
        if(gids.size() != requested_local_ids.size())
            throw std::logic_error("Incorrect number of global cell ids returned from exchange");
        for(int i=0;i<int(requested_local_ids.size());i++){
            int cell_id = requested_local_ids[i];
            long gid = gids[i];
            if(-1 == gid)
                throw std::logic_error("Invalid global cell id (-1) after exchange");
            if(cell_gids[cell_id] != -1)
                throw std::logic_error("Local cell already set: "+std::to_string(cell_id));
            cell_gids[cell_id] = gids[i];
        }
    }
}
bool YogaToTInfinityAdapter::isSameCell(const std::vector<long>& a, const std::vector<long>& b) {
    if(a.size() != b.size())
        return false;
    return std::all_of(a.begin(),a.end(),[&](long x){return 1==std::count(b.begin(),b.end(),x);});
}
void YogaToTInfinityAdapter::cell(int cell_id, std::vector<int>& c) const {
    c.resize(cellLength(cell_id));
    cell(cell_id,c.data());
}
}
