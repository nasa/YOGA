
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
#include <cstdlib>
#include <string>
#include <vector>

inline void MessagePasser::Broadcast(std::string& s, int rootId) const {
    std::vector<char> to_send(s.c_str(), s.c_str() + s.size());
    Broadcast(to_send, rootId);
    s = std::string(to_send.begin(), to_send.end());
}

template <>
inline void MessagePasser::Broadcast<bool>(bool& b, int rootId) const {
    int b_int = int(b);
    Broadcast(b_int, rootId);
    b = bool(b_int);
}

template<typename T>
void MessagePasser::Broadcast(T& value, int rootId) const {
    auto tp = Type(T());
    MPI_Bcast(&value, 1, tp, rootId, getCommunicator());
    freeIfCustomType(tp);
}

template<typename T>
void MessagePasser::Broadcast(std::vector<T>& vec, int vecLength, int rootId) const {
    if ((int) vec.size() != vecLength) {
        if (Rank() == rootId) {
            fprintf(stderr, "MessagePasser::Broadcast: Root is trying to ");
            fprintf(stderr, "send a vector of the wrong length\n");
            fprintf(stderr, "-----------Rank: %i   Vector length: %i   Message size: %i\n",
                    Rank(), (int) vec.size(), vecLength);
            std::exit(0);
        } else
            vec.resize(vecLength);
    }
    if (vecLength != 0) {
        auto tp = Type(T());
        MPI_Bcast(vec.data(), vecLength, tp, rootId, getCommunicator());
        freeIfCustomType(tp);
    }
}

inline void MessagePasser::Broadcast(std::vector<std::string>& strings, int rootId) const {
    size_t length = strings.size();
    Broadcast(length, rootId);

    if(Rank() != rootId){
        strings.resize(length);
    }
    for(size_t i = 0; i < length; i++){
        Broadcast(strings[i], rootId);
    }
}

template<typename T>
void MessagePasser::Broadcast(std::vector<T>& vec, int rootId) const {
    int size = 0;
    if (Rank() == rootId)
        size = (int) vec.size();
    Broadcast(size, rootId);
    if (Rank() != rootId) {
        vec.clear();
        vec.resize(size);
    }
    auto tp = Type(T());
    MPI_Bcast(vec.data(), size, tp, rootId, getCommunicator());
    freeIfCustomType(tp);
}

template<typename T>
void MessagePasser::Broadcast(std::set<T>& set, int rootId) const {
    std::vector<T> vec(set.begin(), set.end());
    Broadcast(vec, rootId);
    set = std::set<T>(vec.begin(), vec.end());
}

template <typename Key, typename Value>
void MessagePasser::Broadcast(std::map<Key, Value>& my_map, int rootId) const {
    std::vector<Key> keys;
    std::vector<Value> values;
    for(auto& pair : my_map){
        keys.push_back(pair.first);
        values.push_back(pair.second);
    }
    Broadcast(keys, rootId);
    Broadcast(values, rootId);

    if(Rank() != rootId){
        for(size_t i = 0; i < keys.size(); i++){
            my_map[keys[i]] = values[i];
        }
    }
}
template <typename Key, typename Value>
void MessagePasser::Broadcast(std::unordered_map<Key, Value>& my_map, int rootId) const {

    std::vector<Key> keys;
    std::vector<Value> values;
    for(auto& pair : my_map){
        keys.push_back(pair.first);
        values.push_back(pair.second);
    }
    Broadcast(keys, rootId);
    Broadcast(values, rootId);

    if(Rank() != rootId){
        for(size_t i = 0; i < keys.size(); i++){
            my_map[keys[i]] = values[i];
        }
    }
}
