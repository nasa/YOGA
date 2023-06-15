#pragma once

template <typename T>
std::vector<std::vector<T>> MessagePasser::Exchange(std::vector<std::vector<T>>&& stuff_for_other_ranks) const {
    auto send_counts = getSendCounts(stuff_for_other_ranks);
    auto recv_counts = getRecvCounts(send_counts);
    auto send_buffer = getSendBuffer(stuff_for_other_ranks, send_counts);
    stuff_for_other_ranks.clear();
    stuff_for_other_ranks.shrink_to_fit();
    auto recv_buffer = getRecvBuffer<T>(recv_counts);
    auto send_displacements = getDisplacementsFromCounts(send_counts);
    auto recv_displacements = getDisplacementsFromCounts(recv_counts);
    auto return_type = Type(T());
    MPI_Alltoallv(send_buffer.data(),
                  send_counts.data(),
                  send_displacements.data(),
                  return_type,
                  recv_buffer.data(),
                  recv_counts.data(),
                  recv_displacements.data(),
                  return_type,
                  communicator);
    send_buffer.clear();
    send_buffer.shrink_to_fit();
    send_counts.clear();
    send_counts.shrink_to_fit();
    send_displacements.clear();
    send_displacements.shrink_to_fit();
    recv_displacements.clear();
    recv_displacements.shrink_to_fit();
    freeIfCustomType(return_type);
    return convertToVectorOfVectors(recv_buffer, recv_counts);
}

template <typename T>
std::vector<std::vector<T>> MessagePasser::Exchange(const std::vector<std::vector<T>>& stuff_for_other_ranks) const {
    auto send_counts = getSendCounts(stuff_for_other_ranks);
    auto recv_counts = getRecvCounts(send_counts);
    auto send_buffer = getSendBuffer(stuff_for_other_ranks, send_counts);
    auto recv_buffer = getRecvBuffer<T>(recv_counts);
    auto send_displacements = getDisplacementsFromCounts(send_counts);
    auto recv_displacements = getDisplacementsFromCounts(recv_counts);
    auto return_type = Type(T());
    MPI_Alltoallv(send_buffer.data(),
                  send_counts.data(),
                  send_displacements.data(),
                  return_type,
                  recv_buffer.data(),
                  recv_counts.data(),
                  recv_displacements.data(),
                  return_type,
                  communicator);
    send_buffer.clear();
    send_buffer.shrink_to_fit();
    send_counts.clear();
    send_counts.shrink_to_fit();
    send_displacements.clear();
    send_displacements.shrink_to_fit();
    recv_displacements.clear();
    recv_displacements.shrink_to_fit();
    freeIfCustomType(return_type);
    return convertToVectorOfVectors(recv_buffer, recv_counts);
}

template <typename Packable>
struct MessagePasser::MapPacker {
    template <typename Packer, typename UnPacker>
    static std::map<int, Packable> Exchange(const MessagePasser& mp,
                                            const std::map<int, Packable>& stuff_for_other_ranks,
                                            Packer packer,
                                            UnPacker unpacker) {
        std::map<int, std::vector<char>> chars_for_ranks;
        for (const auto& stuff : stuff_for_other_ranks) {
            MessagePasser::Message msg(stuff.second, packer);
            chars_for_ranks[stuff.first] = {msg.data(), msg.data() + msg.size()};
        }
        auto chars_from_ranks = mp.Exchange(chars_for_ranks);
        std::map<int, Packable> stuff_from_other_ranks;
        for (auto& chars : chars_from_ranks) {
            MessagePasser::Message msg(chars.second);
            msg.unpack(stuff_from_other_ranks[chars.first], unpacker);
        }
        return stuff_from_other_ranks;
    }
};

template <typename Packable>
struct MessagePasser::MapPacker<std::vector<Packable>> {
    template <typename Packer, typename UnPacker>
    static std::map<int, std::vector<Packable>> Exchange(const MessagePasser& mp,
                                                         const std::map<int, std::vector<Packable>>& stuff_to_swap,
                                                         Packer packer,
                                                         UnPacker unpacker) {
        std::map<int, MessagePasser::Message> messages_to_exchange;
        for (auto& pair : stuff_to_swap) {
            messages_to_exchange[pair.first] = MessagePasser::Message();
            auto& m = messages_to_exchange[pair.first];
            unsigned long count = pair.second.size();
            m.pack(count);
            for (const Packable& t : pair.second) {
                packer(m, t);
            }
        }

        messages_to_exchange = mp.Exchange(messages_to_exchange);

        std::map<int, std::vector<Packable>> out;
        for (auto& pair : messages_to_exchange) {
            int rank = pair.first;
            auto& m = pair.second;
            unsigned long count = 0;
            m.unpack(count);
            for (unsigned long i = 0; i < count; i++) {
                Packable t;
                unpacker(m, t);
                out[rank].push_back(t);
            }
        }
        return out;
    }
};

template <typename Packable, typename Packer, typename UnPacker>
std::map<int, Packable> MessagePasser::Exchange(const std::map<int, Packable>& stuff_for_other_ranks,
                                                Packer packer,
                                                UnPacker unpacker) const {
    return MapPacker<Packable>::Exchange(*this, stuff_for_other_ranks, packer, unpacker);
}

inline std::map<int, MessagePasser::Message> MessagePasser::Exchange(
    std::map<int, Message>& stuff_for_selected_ranks) const {
    std::map<int, std::vector<char>> chars_for_ranks;
    for (const auto& stuff : stuff_for_selected_ranks) {
        auto& msg = stuff.second;
        chars_for_ranks[stuff.first] = {msg.data(), msg.data() + msg.size()};
    }
    auto chars_from_ranks = Exchange(chars_for_ranks);
    std::map<int, Message> stuff_from_other_ranks;
    for (auto& chars : chars_from_ranks) {
        MessagePasser::Message msg(chars.second);
        stuff_from_other_ranks[chars.first] = msg;
    }
    return stuff_from_other_ranks;
}

template <typename Id, typename T>
std::map<Id, std::vector<T>> MessagePasser::Exchange(
    const std::map<Id, std::vector<T>>& stuff_for_selected_ranks) const {
    std::vector<std::vector<T>> vectors_for_all_ranks(NumberOfProcesses());
    for (auto& item : stuff_for_selected_ranks) vectors_for_all_ranks[item.first] = item.second;
    auto vectors_from_all_ranks = Exchange(vectors_for_all_ranks);
    std::map<Id, std::vector<T>> stuff;
    for (Id i = 0; i < Id(NumberOfProcesses()); i++)
        if (vectors_from_all_ranks[i].size() > 0) stuff[i] = vectors_from_all_ranks[i];
    return stuff;
}

inline std::vector<int> MessagePasser::getRecvCounts(const std::vector<int>& send_counts) const {
    std::vector<int> recv_counts(NumberOfProcesses());
    MPI_Alltoall(send_counts.data(), 1, Type(int()), recv_counts.data(), 1, Type(int()), getCommunicator());
    return recv_counts;
}

template <typename T>
std::vector<int> MessagePasser::getSendCounts(const std::vector<std::vector<T>>& stuff_for_other_ranks) const {
    std::vector<int> send_counts(NumberOfProcesses());
    for (int i = 0; i < NumberOfProcesses(); i++) send_counts[i] = bigToInt(stuff_for_other_ranks[i].size());
    return send_counts;
}

template <typename T>
std::vector<T> MessagePasser::getSendBuffer(const std::vector<std::vector<T>>& stuff_for_other_ranks,
                                            const std::vector<int>& send_counts) const {
    int buffer_size = 0;
    for (int n : send_counts) buffer_size += n;
    std::vector<T> send_buffer(buffer_size);
    int index = 0;
    for (auto& stuff : stuff_for_other_ranks)
        for (auto& item : stuff) send_buffer[index++] = item;
    return send_buffer;
}

template <typename T>
std::vector<T> MessagePasser::getRecvBuffer(const std::vector<int>& recv_counts) const {
    int buffer_size = 0;
    for (int n : recv_counts) buffer_size += n;
    return std::vector<T>(buffer_size);
}

inline std::vector<int> MessagePasser::getDisplacementsFromCounts(const std::vector<int>& counts) const {
    std::vector<int> displacements(counts.size(), 0);
    for (size_t i = 1; i < counts.size(); i++) displacements[i] = displacements[i - 1] + counts[i - 1];
    return displacements;
}

template <typename T>
std::vector<std::vector<T>> MessagePasser::convertToVectorOfVectors(const std::vector<T>& recv_buffer,
                                                                    const std::vector<int>& recv_counts) const {
    std::vector<std::vector<T>> stuff_from_other_ranks(NumberOfProcesses());
    int index = 0;
    for (int i = 0; i < NumberOfProcesses(); i++) {
        stuff_from_other_ranks[i].resize(recv_counts[i]);
        for (int j = 0; j < recv_counts[i]; j++) stuff_from_other_ranks[i][j] = recv_buffer[index++];
    }
    return stuff_from_other_ranks;
}
