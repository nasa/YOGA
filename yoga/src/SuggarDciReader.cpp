#include <SuggarDciReader.h>
using namespace YOGA;

void skipKeyAndSpace(const std::string& key, long& pos) {
    pos += 1+key.size();
}
void skipKeyAndRestOfLine(const std::vector<char>& buffer, long& pos) {
    pos += 87; // line records are 80 bytes plus some extra spaces and the letter `P`
}
void skipBytesBetweenReceptors(long& pos){
    pos += 8; // 4 bytes for a `space` saved as an int and a `12`, which is some kind of record marker
}
void fastForwardToKey(const std::string& key,const std::vector<char>& buffer,long& pos){
    std::string s = key;
    int n = s.size();
    for(;pos<long(buffer.size());pos++){
        int matching_chars = 0;
        for(int i=0;i<n;i++) {
            if (s[i] == buffer[pos + i])
                matching_chars++;
            else
                break;
        }
        if(matching_chars == n)
            return;
    }
    throw std::logic_error("Could not find key: "+key);
}
int readAsBinaryInt(const std::vector<char>& buffer, long& pos){
    int byte_count = sizeof(int);
    int n=0;
    memcpy(&n, &buffer[pos],byte_count);
    pos += byte_count;
    return n;
}
double readAsBinaryDouble(const std::vector<char>& buffer,long& pos){
    int byte_count = sizeof(double);
    double x=0;
    memcpy(&x, &buffer[pos],byte_count);
    pos += byte_count;
    return x;
}
bool isCharADigit(char c) { return c < 48 or c > 57; }
int readAsAsciiInt(const std::vector<char>& buffer,long& pos){
    std::string number;
    for(int i=0; i <10; i++){
        char c = buffer[pos+ i];
        if (isCharADigit(c)) {
            pos += i;
            break;
        }
        number.push_back(c);
    }
    int n = Parfait::StringTools::toInt(number);
    return n;
}

std::vector<char> readFileIntoBuffer(std::string filename){
    if(not Parfait::FileTools::doesFileExist(filename)){
        throw std::logic_error("File not found: "+filename);
    }
    FILE* f = fopen(filename.c_str(),"rb");
    fseek(f,0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    std::vector<char> buffer(file_size);
    fread(buffer.data(),1,file_size,f);
    fclose(f);
    return buffer;
}

SuggarDciReader::SuggarDciReader(std::string filename) {
    auto buffer = readFileIntoBuffer(filename);
    std::string key = ":number_grids";
    long pos = 0;
    fastForwardToKey(key, buffer, pos);
    skipKeyAndSpace(key,pos);
    ngrids = readAsAsciiInt(buffer, pos);
    printf("Number of grids: %i\n",ngrids);
    grid_offsets.push_back(0);
    for(int i=0;i<ngrids;i++) {
        key = ":unstr_grid_dims";
        fastForwardToKey(key, buffer, pos);
        skipKeyAndSpace(key, pos);
        int nnodes_in_component = readAsAsciiInt(buffer, pos);
        printf("nnodes in grid %i: %i\n",i, nnodes_in_component);
        long previous_offset = grid_offsets.back();
        grid_offsets.push_back(previous_offset + nnodes_in_component);
    }
    for(int i=0;i<ngrids;i++) {
        readBlockOfReceptors(i,buffer, key, pos);
        key = ":general_drt_iblank";
        readBlockOfIblank(i,buffer,key,pos);
    }
}
void SuggarDciReader::readBlockOfReceptors(int receptor_grid_id,
                                           const std::vector<char>& buffer,
                                           std::string& key,
                                           long& pos) {
    key = ":general_drt_pairs";
    fastForwardToKey(key, buffer, pos);
    long current_key_start = pos;
    skipKeyAndSpace(key, pos);
    int receptors_on_current_grid = readAsAsciiInt(buffer,pos);

    printf("number of receptors for grid %i: %i\n",receptor_grid_id, receptors_on_current_grid);
    pos = current_key_start;
    skipKeyAndRestOfLine(buffer, pos);
    for(int i=0;i< receptors_on_current_grid;i++) {
        readReceptor(receptor_grid_id,buffer, pos);
        skipBytesBetweenReceptors(pos);
    }
}
long SuggarDciReader::getGlobalId(int local_id, int grid_id) {
    return local_id + grid_offsets[grid_id];
}
void SuggarDciReader::readReceptor(int receptor_grid_id, const std::vector<char>& buffer, long& pos) {
    int fortran = 1;
    int node_id = readAsBinaryInt(buffer,pos) - fortran;
    int grid_id = readAsBinaryInt(buffer,pos) - fortran;
    int donor_count = readAsBinaryInt(buffer,pos);
    ndonors += donor_count;
    std::vector<int> donor_ids(donor_count);
    for(int i=0;i<donor_count;i++) {
        donor_ids[i] = readAsBinaryInt(buffer, pos) - fortran;
    }
    pos += 16;  // skip some junk integers
    Dcif::Receptor receptor;
    receptor.gid = getGlobalId(node_id,receptor_grid_id);
    for(auto id:donor_ids)
        receptor.donor_ids.push_back(getGlobalId(id,grid_id));
    receptor.donor_weights.resize(donor_count);
    for(int i=0;i<donor_count;i++)
        receptor.donor_weights[i] = readAsBinaryDouble(buffer, pos);
    receptors.emplace_back(receptor);
}
int SuggarDciReader::nodesInGrid(int grid_id) { return grid_offsets[grid_id+1]-grid_offsets[grid_id]; }
void SuggarDciReader::readBlockOfIblank(int receptor_grid_id, std::vector<char> buffer, std::string key, long& pos) {
    fastForwardToKey(key,buffer,pos);
    long previous_pos = pos;
    skipKeyAndSpace(key,pos);
    int n = readAsAsciiInt(buffer, pos);
    PARFAIT_ASSERT(n== nodesInGrid(receptor_grid_id),"Node count does not match in iblank section");
    pos = previous_pos;
    skipKeyAndRestOfLine(buffer, pos);
    std::map<int,int> suggarToFUN3D;
    suggarToFUN3D[1] = 1;
    suggarToFUN3D[0] = 0;
    suggarToFUN3D[-1] = -1;
    suggarToFUN3D[-2] = -1;
    suggarToFUN3D[-3] = -1;
    suggarToFUN3D[-4] = -1;
    suggarToFUN3D[-5] = -1;
    suggarToFUN3D[101] = -2;
    std::vector<int> iblank_for_grid(n,0);
    for(int i=0;i<n;i++){
        int suggar_iblank = readAsBinaryInt(buffer,pos);
        PARFAIT_ASSERT(1==suggarToFUN3D.count(suggar_iblank),
                       "Unknown suggar iblank value: "+std::to_string(suggar_iblank))
        iblank_for_grid[i] = suggarToFUN3D.at(suggar_iblank);
    }
    node_statuses.insert(node_statuses.end(),iblank_for_grid.begin(),iblank_for_grid.end());
}
