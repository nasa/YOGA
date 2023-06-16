
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

template <typename T>
void MessagePasser::Gather(T value, std::vector<T>& vec, int rootId) const {
    vec.resize(NumberOfProcesses());
    auto tp = Type(value);
    MPI_Gather(&value, 1, tp, vec.data(), 1, tp, rootId, getCommunicator());
    freeIfCustomType(tp);
}
template <typename T>
std::vector<std::vector<T>> MessagePasser::Gather(const std::vector<T>& vec) const {
    std::vector<std::vector<T>> output;
    Gather(vec, output);
    return output;
}

template <typename T>
std::vector<std::vector<T>> MessagePasser::Gather(const std::vector<T>& vec, int rootId) const {
    std::vector<std::vector<T>> output;
    Gather(vec, output, rootId);
    return output;
}

template <typename Id>
Id findMaxId(const std::vector<std::vector<Id>>& ids) {
    Id max = Id(0);
    for (const auto& list_of_ids : ids) {
        for (const auto& id : list_of_ids) {
            max = std::max(id, max);
        }
    }
    return max;
}

template <typename Id, typename T>
std::vector<T> MessagePasser::GatherByOrdinalRange(const std::map<Id, T>& input_data,
                                                   Id start,
                                                   Id end,
                                                   int target_rank) const {
    std::vector<Id> ids;
    std::vector<T> things;
    for (const auto& pair : input_data) {
        Id id = pair.first;
        if (id >= start and id < end) {
            ids.push_back(pair.first);
            things.push_back(pair.second);
        }
    }

    auto gathered_ids = Gather(ids, target_rank);
    auto gathered_things = Gather(things, target_rank);
    ids.clear();
    ids.shrink_to_fit();
    things.clear();
    things.shrink_to_fit();
    auto count = end - start;
    std::vector<T> out;
    if (target_rank == Rank()) out.resize(count);

    for (size_t r = 0; r < gathered_ids.size(); r++) {
        for (size_t i = 0; i < gathered_ids[r].size(); i++) {
            size_t index_into_output_vector = gathered_ids[r][i] - start;
            out.at(index_into_output_vector) = gathered_things[r][i];
        }
    }
    return out;
}

template <typename Id, typename T>
std::vector<T> MessagePasser::GatherByIds(const std::map<Id, T>& input_data, Id start, Id end) const {
    std::vector<Id> ids;
    std::vector<T> things;
    for (const auto& pair : input_data) {
        Id id = pair.first;
        if (id >= start and id < end) {
            ids.push_back(id);
            things.push_back(pair.second);
        }
    }

    auto gathered_ids = Gather(ids);
    auto gathered_things = Gather(things);
    ids.clear();
    ids.shrink_to_fit();
    things.clear();
    things.shrink_to_fit();
    auto count = end - start;
    std::vector<T> out(count);

    for (size_t r = 0; r < gathered_ids.size(); r++) {
        for (size_t i = 0; i < gathered_ids[r].size(); i++) {
            size_t index_into_output_vector = gathered_ids[r][i] - start;
            out.at(index_into_output_vector) = gathered_things[r][i];
        }
    }
    return out;
}

template <typename Id, typename T>
std::vector<T> MessagePasser::GatherByIds(const std::map<Id, T>& input_data) const {
    std::vector<Id> ids;
    std::vector<T> things;
    for (const auto& pair : input_data) {
        ids.push_back(pair.first);
        things.push_back(pair.second);
    }

    auto gathered_ids = Gather(ids);
    auto gathered_things = Gather(things);
    ids.clear();
    ids.shrink_to_fit();
    things.clear();
    things.shrink_to_fit();
    auto max_id = findMaxId(gathered_ids);
    std::vector<T> out(max_id + 1);

    for (size_t r = 0; r < gathered_ids.size(); r++) {
        for (size_t i = 0; i < gathered_ids[r].size(); i++) {
            Id id = gathered_ids[r][i];
            out[id] = gathered_things[r][i];
        }
    }
    return out;
}

template <typename Id, typename T>
std::vector<T> MessagePasser::GatherByIds(const std::map<Id, T>& input_data, int rootId) const {
    std::vector<Id> ids;
    std::vector<T> things;
    for (const auto& pair : input_data) {
        ids.push_back(pair.first);
        things.push_back(pair.second);
    }

    auto gathered_ids = Gather(ids, rootId);
    auto gathered_things = Gather(things, rootId);
    ids.clear();
    ids.shrink_to_fit();
    things.clear();
    things.shrink_to_fit();
    auto max_id = findMaxId(gathered_ids);
    std::vector<T> out;
    if (Rank() == rootId) {
        out.resize(max_id + 1);
        for (size_t r = 0; r < gathered_ids.size(); r++) {
            for (size_t i = 0; i < gathered_ids[r].size(); i++) {
                Id id = gathered_ids[r][i];
                out[id] = gathered_things[r][i];
            }
        }
    }
    return out;
}

template <typename T, typename I>
std::vector<T> MessagePasser::GatherByIds(const std::vector<T>& objects,
                                          const std::vector<I>& global_ids,
                                          int rootId) const {
    if (objects.size() != global_ids.size())
        throw std::domain_error("MessagePasser::GatherByIds requires equal number of objects and ids");
    return GatherAndSort(objects, 1, global_ids, rootId);
}

template <typename T, typename I>
std::vector<T> MessagePasser::GatherByIds(const std::vector<T>& objects, const std::vector<I>& global_ids) const {
    if (objects.size() != global_ids.size())
        throw std::domain_error("MessagePasser::GatherByIds requires equal number of objects and ids");
    return GatherAndSort(objects, 1, global_ids);
}

template <typename T, typename I>
std::vector<T> MessagePasser::GatherAndSort(const std::vector<T>& vec,
                                                   int stride,
                                                   const std::vector<I>& ordinal_ids) const {
    auto output = GatherAndSort(vec, stride, ordinal_ids, 0);
    Broadcast(output, 0);
    return output;
}

template <typename T, typename I>
std::vector<T> MessagePasser::GatherAndSort(const std::vector<T>& vec,
                                                   int stride,
                                                   const std::vector<I>& ordinal_ids,
                                                   int root) const {
    if (vec.size() / stride != ordinal_ids.size())
        throw std::domain_error("MessagePasser::GatherAndSort requires equal number of objects and ids");
    auto objects_from_ranks = Gather(vec, root);
    auto gids_from_all_ranks = Gather(ordinal_ids, root);
    if (Rank() == root) {
        long max_ordinal = 0;
        for (auto& gids_from_rank : gids_from_all_ranks) {
            for(auto gid : gids_from_rank){
                max_ordinal = std::max(max_ordinal, gid);
            }
        }

        size_t total_objects = max_ordinal + 1;

        std::vector<T> output(total_objects * stride);
        for (size_t r = 0; r < objects_from_ranks.size(); r++) {
            auto& gids_from_rank = gids_from_all_ranks[r];
            auto& objects_from_rank = objects_from_ranks[r];
            for (size_t object_index = 0; object_index < gids_from_rank.size(); object_index++) {
                long gid = gids_from_rank[object_index];
                for (int i = 0; i < stride; i++) {
                    output[stride * gid + i] = objects_from_rank[stride * object_index + i];
                }
            }
        }
        return output;
    } else {
        return {};
    }
}

template <typename T>
std::vector<T> MessagePasser::Gather(const std::vector<T>& values, long start, long end) const {
    auto counts = Gather(values.size());
    std::vector<T> to_send;
    long my_offset = 0;
    for (int r = 0; r < Rank(); r++) {
        my_offset += counts[r];
    }

    for (long i = 0; static_cast<size_t>(i) < values.size(); i++) {
        long id = my_offset + i;
        if (id >= start and i < end) {
            to_send.push_back(values[i]);
        }
    }
    auto in = Gather(to_send);
    std::vector<T> out;
    for (auto& outer : in)
        for (auto& t : outer) out.push_back(t);
    return out;
}

template <typename T>
std::vector<T> MessagePasser::Gather(const std::vector<T>& values, long start, long end, int rootId) const {
    auto counts = Gather(values.size());
    std::vector<T> to_send;
    long my_offset = 0;
    for (int r = 0; r < Rank(); r++) {
        my_offset += counts[r];
    }
    for (int i = 0; i < int(values.size()); i++) {
        long id = my_offset + i;
        if (id >= start and i < end) {
            to_send.push_back(values[i]);
        }
    }
    auto in = Gather(to_send, rootId);
    std::vector<T> out;
    for (auto& outer : in)
        for (auto& t : outer) out.push_back(t);
    return out;
}

template <typename T>
void MessagePasser::Gather(T value, std::vector<T>& vec) const {
    vec.resize(NumberOfProcesses());
    auto tp = Type(value);
    MPI_Allgather(&value, 1, tp, vec.data(), 1, Type(value), getCommunicator());
    freeIfCustomType(tp);
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, int send_count, std::vector<T>& recv_vec, int rootId) const {
    if (Rank() == rootId) {
        recv_vec.clear();
        recv_vec.resize(send_count * NumberOfProcesses());
    }
    auto tp = Type(T());
    MPI_Gather(send_vec.data(), send_count, tp, recv_vec.data(), send_count, tp, rootId, getCommunicator());
    freeIfCustomType(tp);
}

template <typename T>
std::vector<T> MessagePasser::Gather(T t, int root_id) const {
    std::vector<T> out(NumberOfProcesses());
    Gather(t, out, root_id);
    return out;
}

template <typename T>
std::vector<T> MessagePasser::Gather(T t) const {
    std::vector<T> out(NumberOfProcesses());
    Gather(t, out);
    return out;
}

template <typename T>
std::vector<std::set<T>> MessagePasser::Gather(const std::set<T>& set) const {
    std::vector<T> output_set(set.begin(), set.end());
    auto all_sets_as_vectors = Gather(output_set);
    std::vector<std::set<T>> all_sets_as_sets;
    for (auto& vec : all_sets_as_vectors) {
        all_sets_as_sets.push_back(std::set<T>(vec.begin(), vec.end()));
    }
    return all_sets_as_sets;
}

template <typename T>
std::vector<std::set<T>> MessagePasser::Gather(const std::set<T>& set, int rootId) const {
    std::vector<T> output_set(set.begin(), set.end());
    auto all_sets_as_vectors = Gather(output_set, rootId);
    std::vector<std::set<T>> all_sets_as_sets;
    for (auto& vec : all_sets_as_vectors) {
        all_sets_as_sets.push_back(std::set<T>(vec.begin(), vec.end()));
    }
    return all_sets_as_sets;
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec,
                           std::vector<T>& recv_vec,
                           std::vector<int>& map,
                           int rootId) const {
    int sendcount = (int)send_vec.size();
    int nproc = NumberOfProcesses();
    std::vector<int> recv_counts(nproc, 0);
    Gather(sendcount, recv_counts, rootId);
    if (Rank() == rootId) {
        map.assign(nproc + 1, 0);
        for (int i = 1; i < nproc + 1; i++) map[i] = map[i - 1] + recv_counts[i - 1];
        recv_vec.resize(map.back());
    }
    auto tp = Type(T());
    MPI_Gatherv((void*)send_vec.data(),
                sendcount,
                tp,
                recv_vec.data(),
                recv_counts.data(),
                map.data(),
                tp,
                rootId,
                getCommunicator());
    freeIfCustomType(tp);
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, std::vector<std::vector<T>>& result, int root_id) const {
    std::vector<int> map;
    std::vector<T> recv;
    if (Rank() == root_id) result.assign(NumberOfProcesses(), std::vector<T>());
    Gather(send_vec, recv, map, root_id);
    if (Rank() == root_id) {
        for (int i = 0; i < NumberOfProcesses(); i++) {
            for (int j = map[i]; j < map[i + 1]; j++) result[i].push_back(recv[j]);
        }
    }
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec, int rootId) const {
    std::vector<int> m;
    Gather(send_vec, recv_vec, m, rootId);
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec) const {
    std::vector<int> m;
    Gather(send_vec, recv_vec, m);
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, std::vector<std::vector<T>>& vec_of_vec_output) const {
    std::vector<int> map;
    std::vector<T> recv_vec;
    Gather(send_vec, recv_vec, map);
    for (unsigned int i = 0; i < map.size() - 1; i++) {
        int n = map[i + 1] - map[i];
        std::vector<T> temp(n);
        vec_of_vec_output.push_back(temp);
        for (int j = 0; j < n; j++) {
            vec_of_vec_output[i][j] = recv_vec[map[i] + j];
        }
    }
}

template <typename T>
void MessagePasser::Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec, std::vector<int>& map) const {
    int sendcount = (int)send_vec.size();
    int nproc = NumberOfProcesses();
    std::vector<int> recv_counts(nproc, 0);
    Gather(sendcount, recv_counts);
    map.assign(nproc + 1, 0);
    for (int i = 1; i < nproc + 1; i++) map[i] = map[i - 1] + recv_counts[i - 1];
    recv_vec.clear();
    recv_vec.resize(map.back());
    auto tp = Type(T());
    MPI_Allgatherv(&send_vec[0], sendcount, tp, &recv_vec[0], &recv_counts[0], &map[0], tp, getCommunicator());
    freeIfCustomType(tp);
}
