
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
#include <stdexcept>
#include <mpi.h>

inline MessagePasser::MessagePasser(MPI_Comm comm) : communicator(comm) {}
inline MPI_Comm MessagePasser::getCommunicator() const { return communicator; }

inline bool MessagePasser::Init() {
    int initialized = 0;
    MPI_Initialized(&initialized);
    bool i_initialized = false;
    if (not initialized) {
        MPI_Init(NULL, NULL);
        i_initialized = true;
    }
    // MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);
    const char* comm_name = "MPI_COMM_WORLD";
    MPI_Comm_set_name(MPI_COMM_WORLD, comm_name);
    return i_initialized;
}

inline MPI_Comm MessagePasser::split(MPI_Comm orig_comm, int color) const {
    MPI_Comm new_comm;
    MPI_Comm_split(orig_comm, color, Rank(), &new_comm);
    return new_comm;
}

inline void MessagePasser::destroyComm() { MPI_Comm_free(&communicator); }

inline void MessagePasser::destroyComm(MPI_Comm this_comm) { MPI_Comm_free(&this_comm); }

inline void MessagePasser::Abort(int code) const { MPI_Abort(communicator, code); }

inline bool MessagePasser::Finalize() {
    int initialized, finalized;
    MPI_Initialized(&initialized);
    bool i_finalized = false;
    if (not initialized) return i_finalized;
    MPI_Finalized(&finalized);
    if (not finalized) {
        MPI_Finalize();
        i_finalized = true;
    }
    return i_finalized;
}

inline int MessagePasser::Rank() const {
    int rank = 0;
    MPI_Comm_rank(getCommunicator(), &rank);
    return rank;
}

inline int MessagePasser::NumberOfProcesses() const {
    int size = 0;
    MPI_Comm_size(getCommunicator(), &size);
    return size;
}

template <typename T>
void MessagePasser::Send(const T& value, int destination) const {
    auto type = Type(value);
    MPI_Send(&value, 1, type, destination, 0, getCommunicator());
    freeIfCustomType(type);
}

template <typename T>
void MessagePasser::Recv(T& value, int source) const {
    MPI_Status status;
    auto type = Type(value);
    MPI_Recv(&value, 1, type, source, 0, communicator, &status);
    freeIfCustomType(type);
}

inline void MessagePasser::Recv(Message& msg, int source) const {
    MPI_Status status;
    MPI_Probe(source, 0, communicator, &status);
    int count = 0;
    MPI_Get_count(&status, MPI_CHAR, &count);
    msg.resize(count);
    MPI_Status other_status;
    MPI_Recv(msg.data(), count, MPI_CHAR, source, 0, communicator, &other_status);
}

template <typename T>
void MessagePasser::Recv(T& value) const {
    MPI_Status status;
    auto type = Type(value);
    MPI_Recv(&value, 1, type, MPI_ANY_SOURCE, 0, getCommunicator(), &status);
    freeIfCustomType(type);
}

inline void MessagePasser::Barrier() const { MPI_Barrier(getCommunicator()); }
inline MessagePasser::MessageStatus MessagePasser::NonBlockingBarrier() const {
    MessageStatus status;
    MPI_Ibarrier(getCommunicator(), status.request());
    return status;
}

inline void MessagePasser::AbortIfAnyRankDoesNotPhoneHomeInTime(double timeout_seconds) const {
    auto status = NonBlockingBarrier();
    bool is_complete = status.waitFor(timeout_seconds);
    if (not is_complete) abort();
}

void MessagePasser::setFinalizeCallback(const std::function<FinalizeCallback*()>& callback_creator) {
    int key;
    auto deleteFunction = [](MPI_Comm comm, int comm_keyval, void* attribute_val, void* extra_state) -> int {
        auto callback_functor = (FinalizeCallback*)extra_state;
        callback_functor->eval();
        delete callback_functor;
        return MPI_SUCCESS;
    };
    MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, deleteFunction, &key, callback_creator());
    MPI_Comm_set_attr(MPI_COMM_SELF, key, nullptr);
    MPI_Comm_free_keyval(&key);
}
