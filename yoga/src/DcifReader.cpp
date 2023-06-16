#include <parfait/ByteSwap.h>
#include <stdio.h>
#include <stdexcept>
#include <parfait/ByteSwap.h>
#include "DcifReader.h"

namespace YOGA {
DcifReader::DcifReader(std::string filename) {
    FILE *f = fopen(filename.c_str(), "rb");
    if (NULL == f) throw std::logic_error("Problem opening file.");
    printf("DcifReader: reading file %s\n", filename.c_str());
    readFile(f);
    convertBackFromFortranIndexing();
    fclose(f);
}

void DcifReader::readFile(FILE *f) {
    fread(&nnodes, 8, 1, f);
    fread(&nfringes, 8, 1, f);
    fread(&ndonors, 8, 1, f);
    fread(&ngrids, 4, 1, f);
    printDcifHeader();
    bool should_swap_bytes = false;
    if(anyCountsAreNegativeOrRidiculouslyBig()){
        should_swap_bytes = true;
        bswap_64(&nnodes);
        bswap_64(&nfringes);
        bswap_64(&ndonors);
        bswap_32(&ngrids);
        printf("Counts seem wrong, swapping endianness.  New counts:\n");
        printDcifHeader();
    }
    if(anyCountsAreNegativeOrRidiculouslyBig()){
        throw std::logic_error("Dcif header doesn't make sense.");
    }

    fringe_ids.assign(nfringes, 0);
    donor_counts.assign(nfringes, 0);
    donor_ids.assign(ndonors, 0);
    donor_weights.assign(ndonors, 0);
    iblank.assign(nnodes, 0);

    fread(fringe_ids.data(), 8, nfringes, f);
    fread(donor_counts.data(), 1, nfringes, f);
    fread(donor_ids.data(), 8, ndonors, f);
    fread(donor_weights.data(), 8, ndonors, f);
    fread(iblank.data(), 1, nnodes, f);

    for(int i=0;i<ngrids;i++){
        int64_t start;
        int64_t end;
        int32_t imesh;
        fread(&start,8,1,f);
        fread(&end,8,1,f);
        fread(&imesh,4,1,f);
        component_grid_start_gid.push_back(start);
        component_grid_end_gid.push_back(end);
        component_grid_imesh.push_back(imesh);
    }

    if(should_swap_bytes){
        swapBytesInVector(fringe_ids);
        swapBytesInVector(donor_counts);
        swapBytesInVector(donor_ids);
        swapBytesInVector(donor_weights);
        swapBytesInVector(iblank);
        swapBytesInVector(component_grid_start_gid);
        swapBytesInVector(component_grid_end_gid);
        swapBytesInVector(component_grid_imesh);
    }
}
void DcifReader::printDcifHeader() const {
    printf("nnodes:       %li\n", long(nnodes));
    printf("nfringes:     %li\n", long(nfringes));
    printf("ndonors:      %li\n", long(ndonors));
    printf("ngrids:       %i\n", ngrids);
}

inline void DcifReader::convertBackFromFortranIndexing() {
    int fortran = 1;
    for(auto& id:fringe_ids) id-=fortran;
    for(auto& id:donor_ids) id-=fortran;
    for(auto& id:component_grid_start_gid) id-=fortran;
    for(auto& id:component_grid_end_gid) id-=fortran;
}
bool DcifReader::anyCountsAreNegativeOrRidiculouslyBig() {
    if(nnodes < 0 or ndonors < 0 or nfringes < 0 or ngrids < 0)
        return true;
    long ridiculously_big = std::numeric_limits<long>::max()/2;
    if(nnodes > ridiculously_big or ndonors > ridiculously_big or nfringes > ridiculously_big)
        return true;
    return false;
}

}