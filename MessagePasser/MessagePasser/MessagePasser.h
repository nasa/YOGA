
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
#include <assert.h>
#include <stdio.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <complex>
#include <functional>
#include <mpi.h>
#include <map>
#include <unordered_map>
#include <cstring>
#include <set>
#include <stack>
#include <limits>
#include <stdint.h>
#include <type_traits>

class MessagePasser {
  public:
    template <typename T, typename U>
    static bool canTypeFitValue(const U value) {
        const intmax_t botT = intmax_t(std::numeric_limits<T>::min());
        const intmax_t botU = intmax_t(std::numeric_limits<U>::min());
        const uintmax_t topT = uintmax_t(std::numeric_limits<T>::max());
        const uintmax_t topU = uintmax_t(std::numeric_limits<U>::max());
        return !((botT > botU && value < static_cast<U>(botT)) || (topT < topU && value > static_cast<U>(topT)));
    }
    template <typename T>
    static int bigToInt(T t) {
        if (not canTypeFitValue<int>(t)) {
            throw std::range_error("MessagePasser could not safely convert a large number to an int");
        }
        return static_cast<int>(t);
    }
#include "MessageStatus.hpp"
#include "Message.h"

    class Promise {
      public:
        inline Promise(Message&& msg) : data(std::make_shared<Message>(std::move(msg))) {}

        auto get() { return data; }
        bool isFulfilled() {
            int flag;
            MPI_Request_get_status(*status.request(), &flag, MPI_STATUS_IGNORE);
            return bool(flag);
        }

        inline MPI_Request* request() { return status.request(); }
        inline void wait() { status.wait(); }
        inline bool waitFor(double remaining_time_to_wait_in_seconds) {
            return status.waitFor(remaining_time_to_wait_in_seconds);
        }
        inline bool isComplete() { return status.isComplete(); }

      private:
        std::shared_ptr<Message> data;
        MessageStatus status;
    };

    class ProbeResult {
      public:
        ProbeResult(bool exists, int source_rank, int size_in)
            : message_exists(exists), source(source_rank), size(size_in) {}

        bool hasMessage() { return message_exists; }
        int sourceRank() { return source; }
        int byteCount() { return size; }

      private:
        bool message_exists;
        int source;
        int size;
    };
    class Window {
      public:
        enum Op { Increment, Decrement, Sum };
        Window(MPI_Comm c);
        void setBuffer(void* b, int n_bytes);
        void initialize();
        MessageStatus get(int rank, int offset, int read_bytes, void* output);

        template <typename T>
        MessageStatus get(Op op, int rank, int offset, T* output);

        template <typename T>
        MessageStatus put(Op op, int rank, int offset, T* input);

        void finalize();

      private:
        MPI_Comm comm;
        void* buffer;
        int bytes;
        MPI_Info info;
        MPI_Win win;
        bool use_barrier_instead_of_win_lock;
    };

    MessagePasser() = delete;
    MessagePasser(MPI_Comm comm);
    static bool Init();
    static bool Finalize();
    int Rank() const;
    int NumberOfProcesses() const;
    MPI_Comm getCommunicator() const;
    static inline MPI_Datatype Type(char value) { return MPI_CHAR; }
    static inline MPI_Datatype Type(int value) { return MPI_INT; }
    static inline MPI_Datatype Type(size_t value) { return MPI_LONG; }
    static inline MPI_Datatype Type(long value) { return MPI_LONG; }
    static inline MPI_Datatype Type(float value) { return MPI_FLOAT; }
    static inline MPI_Datatype Type(double value) { return MPI_DOUBLE; }
    static inline MPI_Datatype Type(int v, int r) { return MPI_2INT; }
    static inline MPI_Datatype Type(long v, int r) { return MPI_LONG_INT; }
    static inline MPI_Datatype Type(float v, int r) { return MPI_FLOAT_INT; }
    static inline MPI_Datatype Type(double v, int r) { return MPI_DOUBLE_INT; }
    static inline std::set<MPI_Datatype> IntrinsicMPIDataTypes() {
        return {
            MPI_CHAR, MPI_INT, MPI_LONG, MPI_FLOAT, MPI_DOUBLE, MPI_2INT, MPI_LONG_INT, MPI_FLOAT_INT, MPI_DOUBLE_INT};
    }
    template <typename ContiguousItem>
    static MPI_Datatype Type(const ContiguousItem& item) {
        static_assert(std::is_trivially_copyable<ContiguousItem>::value,
                      "MessagePasser::Type: must be trivially copyable");
        MPI_Datatype contig_type;
        MPI_Type_contiguous(sizeof(item), MPI_CHAR, &contig_type);
        MPI_Type_commit(&contig_type);
        return contig_type;
    }

    static void freeIfCustomType(MPI_Datatype& type) {
        if (IntrinsicMPIDataTypes().count(type) == 0) {
            MPI_Type_free(&type);
        }
    }

    static inline void check(int error) {
        if (MPI_SUCCESS != error) {
            char s[256];
            int n;
            MPI_Error_string(error, s, &n);
            throw std::logic_error("Mpi error:" + std::string(s));
        }
    }

    struct FinalizeCallback {
        virtual void eval() = 0;
        virtual ~FinalizeCallback() = default;
    };
    static inline void setFinalizeCallback(const std::function<FinalizeCallback*()>& callback_creator);
    void Abort(int exit_code = 1337) const;
    void Barrier() const;
    MessageStatus NonBlockingBarrier() const;

    void AbortIfAnyRankDoesNotPhoneHomeInTime(double seconds_to_wait) const;

    MPI_Comm split(MPI_Comm orig_comm, int color) const;
    void destroyComm();
    void destroyComm(MPI_Comm this_comm);

    ProbeResult Probe() const;

    template <typename StatusType>
    void WaitAll(std::vector<StatusType>& statuses) const;

    template <typename T>
    void Recv(std::vector<T>& value, int source) const;

    template <typename T>
    void Recv(T& value, int source) const;

    void Recv(Message& msg, int source) const;

    template <typename T>
    void Recv(T& value) const;

    template <typename T>
    void Send(const T& value, int source) const;

    template <typename T>
    MessageStatus NonBlockingSend(const T& value, int destination) const;

    template <typename T>
    MessageStatus NonBlockingSend(const T&& value, int destination) const;

    Promise NonBlockingSend(Message&& message, int destination) const;

    template <typename T>
    MessageStatus NonBlockingRecv(const T& value, int source) const;

    template <typename T>
    void Send(const std::vector<T>& vec, int length, int destination) const;

    template <typename T>
    MessageStatus NonBlockingSend(const std::vector<T>& vec, int length, int destination) const;

    template <typename T>
    MessageStatus NonBlockingSend(const std::vector<T>& vec, int destination) const;

    MessageStatus NonBlockingSend(const std::string& s, int destination) const;

    template <typename T>
    MessageStatus NonBlockingRecv(std::vector<T>& vec, int source) const;

    template <typename T>
    MessageStatus NonBlockingRecv(std::vector<T>& vec, int length, int source) const;

    template <typename T>
    MessageStatus NonBlockingRecv(T& d, int source) const;

    template <typename T>
    void Recv(std::vector<T>& vec, int length, int source) const;

    void Recv(std::string& s, int source) const;

    template <typename T>
    void Send(const std::vector<T>& vec, int destination) const;

    template <typename T>
    void Send(const std::vector<std::vector<T>>& vec, int destination) const;

    template <typename T>
    void Recv(std::vector<std::vector<T>>& vec, int source) const;

    template <typename T>
    std::vector<T> Balance(const std::vector<T>& values) const;

    template <typename T>
    std::vector<T> Balance(const std::vector<T>& values, int rank_range_start, int rank_range_end) const;

    template <typename T>
    std::vector<T> Balance(std::vector<T>&& values, int rank_range_start, int rank_range_end) const;

    template <typename T>
    void Gather(T value, std::vector<T>& vec, int rootId) const;

    template <typename T>
    std::vector<T> Gather(const std::vector<T>& values, long start, long end) const;

    template <typename T>
    std::vector<T> Gather(const std::vector<T>& values, long start, long end, int rootId) const;

    template <typename T>
    void Gather(T value, std::vector<T>& vec) const;

    template <typename T>
    std::vector<T> Gather(T value) const;

    template <typename T>
    std::vector<T> Gather(T value, int rootId) const;

    template <typename T>
    std::vector<std::set<T>> Gather(const std::set<T>& set) const;

    template <typename T>
    std::vector<std::set<T>> Gather(const std::set<T>& set, int rootId) const;

    template <typename T>
    std::vector<std::vector<T>> Gather(const std::vector<T>& vec) const;

    template <typename T>
    std::vector<std::vector<T>> Gather(const std::vector<T>& vec, int rootId) const;

    template <typename Id, typename T>
    std::vector<T> GatherByOrdinalRange(const std::map<Id, T>& input_data, Id start, Id end, int rank) const;

    template <typename Id, typename T>
    std::vector<T> GatherByIds(const std::map<Id, T>& input_data, Id start, Id end) const;

    template <typename Id, typename T>
    std::vector<T> GatherByIds(const std::map<Id, T>& input_data) const;

    template <typename Id, typename T>
    std::vector<T> GatherByIds(const std::map<Id, T>& input_data, int rootId) const;

    template <typename T, typename I>
    std::vector<T> GatherByIds(const std::vector<T>& vec, const std::vector<I>& global_ids, int rootId) const;

    template <typename T, typename I>
    std::vector<T> GatherByIds(const std::vector<T>& vec, const std::vector<I>& global_ids) const;

    template <typename T, typename I>
    std::vector<T> GatherAndSort(const std::vector<T>& vec,
                                 int stride,
                                 const std::vector<I>& ordinal_ids,
                                 int root) const;

    template <typename T, typename I>
    std::vector<T> GatherAndSort(const std::vector<T>& vec, int stride, const std::vector<I>& ordinal_ids) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, int send_count, std::vector<T>& recv_vec, int rootId) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec, std::vector<int>& map, int rootId) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec, int rootId) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<std::vector<T>>& result, int root_id) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec, std::vector<int>& map) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<T>& recv_vec) const;

    template <typename T>
    void Gather(const std::vector<T>& send_vec, std::vector<std::vector<T>>& vec_of_vec_output) const;

    void Broadcast(std::string& s, int rootId) const;
    void Broadcast(std::vector<std::string>& s, int rootId) const;

    template <typename T>
    void Broadcast(T& value, int rootId) const;

    template <typename T>
    void Broadcast(std::vector<T>& vec, int vecLength, int rootId) const;

    template <typename T>
    void Broadcast(std::vector<T>& vec, int rootId) const;

    template <typename T>
    void Broadcast(std::set<T>& set, int rootId) const;

    template <typename Key, typename Value>
    void Broadcast(std::map<Key, Value>& my_map, int rootId) const;

    template <typename Key, typename Value>
    void Broadcast(std::unordered_map<Key, Value>& my_map, int rootId) const;

    template <typename T>
    T Scatter(const std::vector<T>& vec, int rootId) const;

    template <typename T>
    std::vector<T> Scatter(const std::vector<std::vector<T>>& vec, int rootId) const;

    template <typename T>
    void Scatter(const std::vector<T>& vec, T& recv_value, int rootId) const;

    template <typename T>
    void Scatter(const std::vector<T>& vec, std::vector<T>& recv_vec, int rootId) const;

    template <typename T>
    void Scatterv(const std::vector<T>& vec, std::vector<T>& recv_vec, int rootId) const;

    template <typename T>
    void ElementalSum(std::vector<T>& vec, int root) const;

    template <typename T>
    T ParallelSum(T value, int rootId) const;

    template <typename T>
    T ParallelSum(const T& value) const;

    template <typename T>
    T ParallelAverage(const T& value) const;

    bool ParallelOr(bool b) const;

    bool ParallelAnd(bool b) const;

    template <typename T>
    std::set<T> ParallelUnion(const std::set<T>& set) const;

    template <typename T>
    std::complex<T> ParallelSum(const std::complex<T>& value) const;

    template <typename T>
    std::vector<T> ParallelMax(const std::vector<T>& vec, int rootId) const;

    template <typename T>
    T ParallelMax(T value, int rootId) const;

    template <typename T>
    T ParallelMax(T value) const;

    template <typename T>
    std::vector<T> ParallelMin(const std::vector<T>& vec, int rootId) const;

    template <typename T>
    T ParallelMin(T value, int rootId) const;

    template <typename T>
    T ParallelMin(T value) const;

    template <typename T>
    T ParallelRankOfMax(T value) const;

    template <typename T>
    std::vector<T> ElementalMax(const std::vector<T>& vec, int root) const;

    template <typename T>
    std::vector<T> ElementalMax(const std::vector<T>& vec) const;

    template <typename T, typename Op>
    T Reduce(const T& value, Op op);

    template <typename T, typename Op>
    std::vector<T> Reduce(const std::vector<T>& value, Op op);

    template <typename Id, typename Summable, typename GetOwner>
    std::map<Id, Summable> SumAtId(const std::map<Id, Summable>& stuff_to_sum, GetOwner get_owner) const;

    template <typename T>
    std::vector<std::vector<T>> Exchange(const std::vector<std::vector<T>>& stuff_for_other_ranks) const;

    template <typename Packable, typename Packer, typename UnPacker>
    std::map<int, Packable> Exchange(const std::map<int, Packable>& stuff_for_other_ranks,
                                     Packer packer,
                                     UnPacker unpacker) const;

    template <typename T>
    std::vector<std::vector<T>> Exchange(std::vector<std::vector<T>>&& stuff_for_other_ranks) const;

    template <typename Id, typename T>
    std::map<Id, std::vector<T>> Exchange(const std::map<Id, std::vector<T>>& stuff_for_other_ranks) const;

    std::map<int, Message> Exchange(std::map<int, Message>& stuff_for_other_ranks) const;

  private:
    MPI_Comm communicator;
    static std::stack<MPI_Datatype>& typeStack() {
        static std::stack<MPI_Datatype> ts;
        return ts;
    }

    std::vector<int> getRecvCounts(const std::vector<int>& send_counts) const;
    template <typename T>
    std::vector<int> getSendCounts(const std::vector<std::vector<T>>& stuff_for_other_ranks) const;
    template <typename T>
    std::vector<T> getSendBuffer(const std::vector<std::vector<T>>& stuff_for_other_ranks,
                                 const std::vector<int>& send_counts) const;
    template <typename T>
    std::vector<T> getRecvBuffer(const std::vector<int>& recv_counts) const;
    std::vector<int> getDisplacementsFromCounts(const std::vector<int>& counts) const;
    template <typename T>
    std::vector<std::vector<T>> convertToVectorOfVectors(const std::vector<T>& recv_buffer,
                                                         const std::vector<int>& recv_counts) const;
    template <typename T, typename Op>
    void Reduce(const T* input, T* output, int count, Op op);

    template <typename Packable>
    struct MapPacker;
};

#define mp_rootprint(...)        \
    {                            \
        if (mp.Rank() == 0) {    \
            printf(__VA_ARGS__); \
            fflush(stdout);      \
        }                        \
    }

#include "MessagePasserProbe.hpp"
#include "MessagePasserSends.hpp"
#include "MessagePasserRecvs.hpp"
#include "MessagePasserWait.hpp"
#include "MessagePasserBalance.hpp"
#include "MessagePasserBroadcasts.hpp"
#include "MessagePasserScatters.hpp"
#include "MessagePasserReductions.hpp"
#include "MessagePasser.hpp"
#include "MessagePasserGathers.hpp"
#include "MessagePasserAllToAll.hpp"
