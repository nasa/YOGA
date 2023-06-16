#pragma once
#include <parfait/LinearPartitioner.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/ByteSwap.h>

namespace YOGA {

class PartVectorIO {
  public:
    PartVectorIO(MessagePasser mp, const std::vector<long>& owned_global_ids,bool swap_bytes = false)
        : mp(mp),
          swap_bytes(swap_bytes),
          my_gids(owned_global_ids)
    {}
    void exportPartVector(const std::string filename) {
        writeHeader(filename);

        auto getChunk = [&](long start, long end) {
            auto gids = extractIdsInRange(start, end);
            std::vector<int> rank_of_node(gids.size(), mp.Rank());
            for (auto& gid : gids) gid -= start;
            return mp.GatherAndSort(rank_of_node, 1, gids, 0);
        };

        auto putChunk = [&](std::vector<int>& chunk, long start, long end) {
            if (0 == mp.Rank()) {
                convertToFortranIndexing(chunk);
                if(swap_bytes)
                    swapBytes(chunk);
                fwrite(chunk.data(), 4, chunk.size(), f);
            }
        };

        if (0 == mp.Rank()) printf("Export part file\n");
        processInChunks(getChunk, putChunk);

        closeFile();
    }

    std::vector<int> importPartVector(const std::string filename) {
        openFileAndVerifyHeader(filename);

        auto getChunk = [&](long start, long end) {
            std::vector<int> chunk;
            if (0 == mp.Rank()) {
                long n = end - start;
                chunk.resize(n);
                fread(chunk.data(), 4, n, f);
                if(swap_bytes)
                    swapBytes(chunk);
                convertFromFortranIndexing(chunk);
            }
            return chunk;
        };

        std::vector<int> part(my_gids.size(), 0);

        auto putChunk = [&](std::vector<int>& chunk, long start, long end) {
            mp.Broadcast(chunk, 0);
            for (size_t i = 0; i < my_gids.size(); i++) {
                long gid = my_gids[i];
                if (gid >= start and gid < end) {
                    int offset = gid - start;
                    part[i] = chunk[offset];
                }
            }
        };

        if (0 == mp.Rank()) printf("Import part file\n");
        processInChunks(getChunk, putChunk);

        closeFile();
        return part;
    }

  private:
    MessagePasser mp;
    bool swap_bytes;
    FILE* f = nullptr;
    const std::vector<long>& my_gids;

    void convertToFortranIndexing(std::vector<int>& v){
        for(int& x:v)
            ++x;
    }

    void convertFromFortranIndexing(std::vector<int>& v){
        for(int& x:v)
            --x;
    }

    template <typename Getter, typename Putter>
    void processInChunks(Getter getChunk, Putter putChunk) {
        int chunk_size = 1e5;
        long total_nodes = mp.ParallelSum(long(my_gids.size()));
        long nchunks = total_nodes / chunk_size + 1;
        for (int chunk_id = 0; chunk_id < nchunks; chunk_id++) {
            auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk_id, total_nodes, nchunks);
            if (0 == mp.Rank()) printf("Processing chunk(%li,%li)\n", range.start, range.end);
            auto chunk = getChunk(range.start, range.end);
            putChunk(chunk, range.start, range.end);
        }
    }

    void writeHeader(const std::string& filename) {
        f = fopen(filename.c_str(), "wb");
        int nranks = mp.NumberOfProcesses();
        long total_nodes = mp.ParallelSum(my_gids.size());
        if (0 == mp.Rank()) {
            if(swap_bytes){
                bswap_32(&nranks);
                bswap_64(&total_nodes);
            }
            fwrite(&nranks, 4, 1, f);
            fwrite(&total_nodes, 8, 1, f);
        }
    }

    void openFileAndVerifyHeader(const std::string& filename) {
        if (0 == mp.Rank()) {
            f = fopen(filename.c_str(), "rb");
            int nranks;
            fread(&nranks, 4, 1, f);
            long nnodes;
            fread(&nnodes, 8, 1, f);
            if(swap_bytes){
                bswap_32(&nranks);
                bswap_64(&nnodes);
            }
            printf("%s: has %i ranks\n", filename.c_str(), nranks);
            printf("total nodes: %li\n", nnodes);
            if (mp.NumberOfProcesses() != nranks) throw std::logic_error("Mismatch in rank count ");
        }
    }

    void closeFile() {
        if (0 == mp.Rank()) {
            fclose(f);
        }
    }

    std::vector<long> extractIdsInRange(long begin, long end) {
        std::vector<long> extracted;
        for (auto gid : my_gids)
            if (gid >= begin and gid < end) extracted.push_back(gid);
        return extracted;
    }

    void swapBytes(std::vector<int>& v){
        for(int& x:v)
            bswap_32(&x);
    }
};
}