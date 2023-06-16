
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space Administration. 
// No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
// 
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the 
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0. 
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed 
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.
template <typename T>
T MessagePasser::Scatter(const std::vector<T>& vec, int rootId) const {
    T out;
    Scatter(vec, out, rootId);
    return out;
}
template <typename T>
std::vector<T> MessagePasser::Scatter(const std::vector<std::vector<T>>& vec, int rootId) const {
    std::vector<size_t> lengths;
    if(Rank() == rootId){
        lengths.resize(NumberOfProcesses());
        for(int r = 0; r < NumberOfProcesses(); r++){
            lengths[r] = vec[r].size();
        }
    }
    size_t incoming_size = Scatter(lengths, rootId);

    std::vector<T> out(incoming_size);
    if(Rank() != rootId){
        Recv(out, bigToInt(out.size()), rootId);
    } else {
        std::vector<MessagePasser::MessageStatus> statuses;
        for(int r = 0; r < NumberOfProcesses(); r++){
            if(r == rootId) continue;
            statuses.push_back(NonBlockingSend(vec[r], r));
        }
        WaitAll(statuses);
        out = vec[rootId];
    }
    return out;
}

template<typename T>
void MessagePasser::Scatter(const std::vector<T>& vec, T& recv_value, int rootId) const {
    auto type = Type(T());
    MPI_Scatter(vec.data(), 1, type, &recv_value, 1, type, rootId, getCommunicator());
    freeIfCustomType(type);
}

template<typename T>
void MessagePasser::Scatter(const std::vector<T>& vec, std::vector<T>& recv_vec, int rootId) const {
    int size = 0;
    if (Rank() == rootId) {
        int total_length = (int) vec.size();
        assert(total_length % NumberOfProcesses() == 0); // must scatter equal amounts
        size = total_length / NumberOfProcesses();
    }
    Broadcast(size, rootId);
    recv_vec.resize(size);
    auto type = Type(T());
    MPI_Scatter(vec.data(), size, type, recv_vec.data(), size, type, rootId,
                getCommunicator());
    freeIfCustomType(type);
}

template<typename T>
void MessagePasser::Scatterv(const std::vector<T>& vec, std::vector<T>& recv_vec, int rootId) const {
    std::vector<int> sendcounts;
    std::vector<int> displs;
    int local_size = 0;
    if (Rank() == rootId) {
        int total_size = (int) vec.size();
        int nproc = NumberOfProcesses();
        sendcounts.assign(nproc, total_size / nproc);
        for (int i = 0; i < nproc; i++)
            if (i < (total_size % nproc))
                sendcounts[i]++;
        displs.assign(nproc, 0);
        for (int i = 1; i < nproc; i++)
            displs[i] = displs[i - 1] + sendcounts[i - 1];
    }
    Scatter(sendcounts, local_size, rootId);
    recv_vec.resize(local_size);
    //convert sendcounts etc to char numbering
    auto type = Type(T());
    MPI_Scatterv(vec.data(), sendcounts.data(), displs.data(), type,
                 recv_vec.data(), local_size, type, rootId, getCommunicator());
    freeIfCustomType(type);
}
