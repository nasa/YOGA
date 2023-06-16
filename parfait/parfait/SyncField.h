
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
#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <MessagePasser/MessagePasser.h>
#include "SyncPattern.h"
#include "Throw.h"

namespace Parfait {
template <typename T>
class SyncWrapper {
  public:
    SyncWrapper(std::vector<T>& Q, const std::map<long, int>& global_to_local)
        : Q(Q), global_to_local(global_to_local) {
        static_assert(!std::is_same<T, bool>::value, "syncing std::vector<bool> is not allowed.");
    }

    inline T& getEntry(long global_id) {
        if (global_to_local.count(global_id) == 0) {
            PARFAIT_THROW("Can not get entry for global id " + std::to_string(global_id));
        }
        PARFAIT_ASSERT_KEY(global_to_local, global_id);
        auto local = global_to_local.at(global_id);
        return Q[local];
    }

    inline void setEntry(long global_id, const T& q) {
        PARFAIT_ASSERT_KEY(global_to_local, global_id);
        auto local = global_to_local.at(global_id);
        Q[local] = q;
    }

  private:
    std::vector<T>& Q;
    const std::map<long, int>& global_to_local;
};

template <typename T>
struct SyncerBuffer {
    std::unordered_map<int, std::vector<T>> buffer;
    std::vector<MessagePasser::MessageStatus> statuses;
};

template <typename T, typename Map>
class StridedSyncer {
  public:
    StridedSyncer(MessagePasser mp,
                  T* source,
                  T* target,
                  const Parfait::SyncPattern& sync_pattern,
                  const Map& source_g2l,
                  const Map& target_g2l,
                  int stride)
        : mp(mp),
          source_values(source),
          target_values(target),
          stride(stride),
          sync_pattern(sync_pattern),
          source_g2l(source_g2l),
          target_g2l(target_g2l) {}
    StridedSyncer(
        MessagePasser mp, T* data, const Parfait::SyncPattern& sync_pattern, const Map& global_to_local, int stride)
        : StridedSyncer(mp, data, data, sync_pattern, global_to_local, global_to_local, stride) {}

    void start() {
        for (auto& pair : sync_pattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            recv.buffer[source].resize(recv_list.size() * stride);
            auto s = mp.NonBlockingRecv(recv.buffer[source], recv_list.size() * stride, source);
            recv.statuses.push_back(s);
        }
        for (auto& pair : sync_pattern.send_to) {
            auto& destination = pair.first;
            auto& send_list = pair.second;
            send.buffer[destination].resize(send_list.size() * stride);
            for (size_t r = 0; r < send_list.size(); r++) {
                long global = send_list[r];
                int local = source_g2l.at(global);
                for (int i = 0; i < stride; i++) {
                    send.buffer[destination][stride * r + i] = source_values[stride * local + i];
                }
            }
            auto s = mp.NonBlockingSend(send.buffer[destination], destination);
            send.statuses.push_back(s);
        }
    }
    void finish() {
        mp.WaitAll(recv.statuses);

        // We could unpack recv messages as they arrive and not wait all.
        // That could minutely speed this up.
        for (auto& pair : sync_pattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            for (int r = 0; r < int(recv_list.size()); r++) {
                auto global_id = recv_list[r];
                int local = target_g2l.at(global_id);
                for (int i = 0; i < stride; i++) {
                    target_values[local * stride + i] = recv.buffer[source][r * stride + i];
                }
            }
        }

        mp.WaitAll(send.statuses);

        recv = {};
        send = {};
    }

  private:
    MessagePasser mp;
    T* source_values;
    T* target_values;
    int stride;
    const Parfait::SyncPattern& sync_pattern;
    const Map& source_g2l;
    const Map& target_g2l;
    SyncerBuffer<T> recv;
    SyncerBuffer<T> send;
};

template <typename T, typename Field>
class Syncer {
  public:
    Syncer(MessagePasser mp, Field& f, const Parfait::SyncPattern& syncPattern)
        : mp(mp), f(f), syncPattern(syncPattern) {}

    void start() {
        for (auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            recv.buffer[source].resize(recv_list.size());
            auto s = mp.NonBlockingRecv(recv.buffer[source], recv_list.size(), source);
            recv.statuses.push_back(s);
        }
        for (auto& pair : syncPattern.send_to) {
            auto& destination = pair.first;
            auto& send_list = pair.second;
            for (auto t : send_list) {
                send.buffer[destination].push_back(f.getEntry(t));
            }
            auto s = mp.NonBlockingSend(send.buffer[destination], destination);
            send.statuses.push_back(s);
        }
    }

    void finish() {
        mp.WaitAll(recv.statuses);

        for (auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            for (int i = 0; i < int(recv_list.size()); i++) {
                auto global_id = recv_list[i];
                f.setEntry(global_id, recv.buffer[source][i]);
            }
        }

        mp.WaitAll(send.statuses);

        recv = {};
        send = {};
    }

  private:
    MessagePasser mp;
    Field& f;
    const Parfait::SyncPattern& syncPattern;
    SyncerBuffer<T> recv;
    SyncerBuffer<T> send;
};

template <typename T, typename Field>
class StridedFieldSyncer {
  public:
    StridedFieldSyncer(MessagePasser mp, Field& f, const Parfait::SyncPattern& syncPattern)
        : mp(mp), f(f), syncPattern(syncPattern) {}

    void start() {
        for (const auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            recv.buffer[source].resize(recv_list.size() * f.stride());
            auto s = mp.NonBlockingRecv(recv.buffer[source], source);
            recv.statuses.push_back(s);
        }
        for (const auto& pair : syncPattern.send_to) {
            auto& destination = pair.first;
            auto& send_list = pair.second;
            for (auto t : send_list) {
                for (int i = 0; i < f.stride(); ++i) {
                    send.buffer[destination].push_back(f.getEntry(t, i));
                }
            }
            auto s = mp.NonBlockingSend(send.buffer[destination], destination);
            send.statuses.push_back(s);
        }
    }

    void finish() {
        mp.WaitAll(recv.statuses);

        for (const auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            for (int local = 0; local < int(recv_list.size()); local++) {
                auto global_id = recv_list[local];
                for (int i = 0; i < f.stride(); ++i) {
                    f.setEntry(global_id, i, recv.buffer[source][local * f.stride() + i]);
                }
            }
        }

        mp.WaitAll(send.statuses);

        recv = {};
        send = {};
    }

  private:
    MessagePasser mp;
    Field& f;
    const Parfait::SyncPattern& syncPattern;
    SyncerBuffer<T> recv;
    SyncerBuffer<T> send;
};

template <typename ElementRefGetter, typename Packer, typename UnPacker>
class NonContiguousSyncer {
  public:
    NonContiguousSyncer(MessagePasser mp,
                        ElementRefGetter getElementReference,
                        Packer packer,
                        UnPacker unpacker,
                        const Parfait::SyncPattern& syncPattern)
        : mp(mp),
          getElementReference(getElementReference),
          packer(packer),
          unpacker(unpacker),
          syncPattern(syncPattern) {}

    void sync() {
        std::map<int, MessagePasser::Message> recv_msgs;
        std::map<int, MessagePasser::Message> send_msgs;
        std::vector<MessagePasser::Promise> send_promises;
        for (auto& pair : syncPattern.send_to) {
            auto& destination = pair.first;
            auto& send_list = pair.second;
            for (auto t : send_list) {
                auto ref = getElementReference(t);
                send_msgs[destination].pack(MessagePasser::Message(ref, packer));
            }
            send_promises.emplace_back(mp.NonBlockingSend(std::move(send_msgs[destination]), destination));
        }
        for (auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            mp.Recv(recv_msgs[source], source);
        }

        for (auto& pair : syncPattern.receive_from) {
            auto& source = pair.first;
            auto& recv_list = pair.second;
            auto& msg = recv_msgs[source];
            for (int i = 0; i < int(recv_list.size()); i++) {
                auto global_id = recv_list[i];
                auto& ref = getElementReference(global_id);
                MessagePasser::Message item;
                msg.unpack(item);
                item.unpack(ref, unpacker);
            }
        }

        mp.WaitAll(send_promises);
    }

  private:
    MessagePasser mp;
    ElementRefGetter getElementReference;
    Packer packer;
    UnPacker unpacker;
    const Parfait::SyncPattern& syncPattern;
};

class SyncerFactory {
  public:
    template <typename ElementRefGetter, typename Packer, typename UnPacker>
    static NonContiguousSyncer<ElementRefGetter, Packer, UnPacker> build(MessagePasser mp,
                                                                         ElementRefGetter getElementReference,
                                                                         Packer packer,
                                                                         UnPacker unpacker,
                                                                         const Parfait::SyncPattern& syncPattern) {
        return NonContiguousSyncer<ElementRefGetter, Packer, UnPacker>(
            mp, getElementReference, packer, unpacker, syncPattern);
    }
};

template <typename T, typename Field>
void syncField(MessagePasser mp, Field& f, const Parfait::SyncPattern& syncPattern) {
    Syncer<T, Field> syncer(mp, f, syncPattern);
    syncer.start();
    syncer.finish();
}

template <typename T>
void syncVector(MessagePasser mp,
                std::vector<T>& vec,
                const std::map<long, int>& global_to_local,
                const Parfait::SyncPattern& sync_pattern) {
    SyncWrapper<T> syncer(vec, global_to_local);
    syncField<T>(mp, syncer, sync_pattern);
}

template <typename T, typename Map>
void syncStridedPointer(
    MessagePasser mp, T* data, const Map& global_to_local, const Parfait::SyncPattern& sync_pattern, int stride) {
    StridedSyncer<T, Map> syncer(mp, data, sync_pattern, global_to_local, stride);
    syncer.start();
    syncer.finish();
}

template <typename T, typename Map>
void syncStridedVector(MessagePasser mp,
                       std::vector<T>& data,
                       const Map& global_to_local,
                       const Parfait::SyncPattern& sync_pattern,
                       int stride) {
    static_assert(!std::is_same<T, bool>::value, "syncing std::vector<bool> is not allowed.");
    StridedSyncer<T, Map> syncer(mp, data.data(), sync_pattern, global_to_local, stride);
    syncer.start();
    syncer.finish();
}

template <typename T, typename Field>
void syncStridedField(MessagePasser mp, Field& f, const Parfait::SyncPattern& sync_pattern) {
    StridedFieldSyncer<T, Field> syncer(mp, f, sync_pattern);
    syncer.start();
    syncer.finish();
}
}