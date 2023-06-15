#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Adt3dExtent.h>
#include <Tracer.h>
#include "PartitionInfo.h"
#include "TransferNode.h"
#include "YogaMesh.h"

namespace YOGA {

template <int N>
class TransferCell {
  public:
    TransferCell() {}
    TransferCell(const std::array<int, N>& vertices, int id, int owner)
        : nodeIds(vertices), cellId(id), owningRank(owner) {}
    std::array<int, N> nodeIds;
    int cellId;
    int owningRank;
};

class VoxelFragment {
  public:
    VoxelFragment() {}
    VoxelFragment(const YogaMesh& m,
                  const std::vector<BoundaryConditions>& node_bcs,
                  const std::vector<int>& overlapping_cell_ids,
                  int rank) {
       fillFragment(m,node_bcs,overlapping_cell_ids,rank);
    }

    static void pack(MessagePasser::Message& msg,VoxelFragment& fragment){
        msg.pack(fragment.transferNodes);
        msg.pack(fragment.transferTets);
        msg.pack(fragment.transferPyramids);
        msg.pack(fragment.transferPrisms);
        msg.pack(fragment.transferHexs);

    }
    static void unpack(MessagePasser::Message& msg,VoxelFragment& fragment){
        msg.unpack(fragment.transferNodes);
        msg.unpack(fragment.transferTets);
        msg.unpack(fragment.transferPyramids);
        msg.unpack(fragment.transferPrisms);
        msg.unpack(fragment.transferHexs);
    }

    void fillFragment(const YogaMesh& m,
        const std::vector<BoundaryConditions>& node_bcs,
        const std::vector<int>& overlapping_cell_ids,
        int rank) {
        std::vector<int> fragment_node_id(m.nodeCount(), NOT_IN_FRAGMENT);
        int nodes_in_fragment = 0;
        CellCounts counts;
        for (int id : overlapping_cell_ids) {
            int cell_size = m.numberOfNodesInCell(id);
            auto cell_nodes = m.cell_ptr(id);
            updateCellCounts(cell_size, counts);
            for (int j=0;j<cell_size;j++) {
                int node_id = cell_nodes[j];
                if (fragment_node_id[node_id] == NOT_IN_FRAGMENT) {
                    fragment_node_id[node_id] = nodes_in_fragment++;
                }
            }
        }
        transferNodes.resize(nodes_in_fragment);
        for (int i = 0; i < m.nodeCount(); ++i) {
            if (fragment_node_id[i] != NOT_IN_FRAGMENT) {
                transferNodes[fragment_node_id[i]] = TransferNode(m.globalNodeId(i),
                                                                  m.getNode<double>(i),
                                                                  0.0,
                                                                  m.getAssociatedComponentId(i), m.nodeOwner(i));
            }
        }
        transferTets.resize(counts.tet);
        transferPyramids.resize(counts.pyramid);
        transferPrisms.resize(counts.prism);
        transferHexs.resize(counts.hex);
        CellCounts added_so_far;
        for (int id : overlapping_cell_ids) {
            int cell_size = m.numberOfNodesInCell(id);
            auto cell_ptr = m.cell_ptr(id);
            storeCell(m, cell_size, cell_ptr, id,rank, fragment_node_id, added_so_far);
        }
    }

    std::vector<TransferNode> transferNodes;
    std::vector<TransferCell<4>> transferTets;
    std::vector<TransferCell<5>> transferPyramids;
    std::vector<TransferCell<6>> transferPrisms;
    std::vector<TransferCell<8>> transferHexs;

    struct CellCounts {
        int tet = 0;
        int pyramid = 0;
        int prism = 0;
        int hex = 0;
    };

    int NOT_IN_FRAGMENT = -1;

    void updateCellCounts(int size, CellCounts& counts) {
        switch (size) {
            case 4:
                counts.tet++;
                return;
            case 5:
                counts.pyramid++;
                return;
            case 6:
                counts.prism++;
                return;
            case 8:
                counts.hex++;
        }
    }

    inline void storeCell(const YogaMesh& m,
                          int cell_size,
                          const int* cell_ptr,
                          int cellId,
                          int rank,
                          const std::vector<int>& fragment_node_id,
                          CellCounts& added_so_far) {
        if (4 == cell_size) {
            int tet_id = added_so_far.tet++;
            auto& tet = transferTets[tet_id];
            tet.cellId = cellId;
            tet.owningRank = rank;
            convertCellToFragmentIndexing<4>(tet.nodeIds, cell_ptr, fragment_node_id);
        } else if (5 == cell_size) {
            int pyramid_id = added_so_far.pyramid++;
            auto& pyramid = transferPyramids[pyramid_id];
            pyramid.cellId = cellId;
            pyramid.owningRank = rank;
            convertCellToFragmentIndexing<5>(pyramid.nodeIds, cell_ptr, fragment_node_id);
        } else if (6 == cell_size) {
            int prism_id = added_so_far.prism++;
            auto& prism = transferPrisms[prism_id];
            prism.cellId = cellId;
            prism.owningRank = rank;
            convertCellToFragmentIndexing<6>(prism.nodeIds, cell_ptr, fragment_node_id);
        } else if (8 == cell_size) {
            int hex_id = added_so_far.hex++;
            auto& hex = transferHexs[hex_id];
            hex.cellId = cellId;
            hex.owningRank = rank;
            convertCellToFragmentIndexing<8>(hex.nodeIds, cell_ptr, fragment_node_id);
        } else {
            printf("Error cell length is %i only tet/pyramid/prism/hex are supported\n", cell_size);
            fflush(stdout);
            throw std::logic_error("Invalid cell type");
        }
    }

    template <int N>
    void convertCellToFragmentIndexing(std::array<int, N>& transfer_cell,
                                       const int* input_cell,
                                       const std::vector<int>& fragment_node_id) {
        for (int i = 0; i < N; i++) {
            transfer_cell[i] = fragment_node_id[input_cell[i]];
        }
    }
};
}
