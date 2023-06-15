
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
#include <limits>
#include <vector>
#include <complex>

template<typename T>
void MessagePasser::ElementalSum(std::vector<T>& vec,int root) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T t = 0;
    std::vector<T> source = vec;
    MPI_Reduce(source.data(),vec.data(),bigToInt(source.size()),Type(t),MPI_SUM,root,getCommunicator());
}

template<typename T>
T MessagePasser::ParallelSum(T value, int rootId) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T sum = 0;
    MPI_Reduce(&value, &sum, 1, Type(value), MPI_SUM, rootId, getCommunicator());
    return sum;
}

template<typename T>
T MessagePasser::ParallelMax(T value, int rootId) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T tmp = value;
    T max = value;
    MPI_Reduce(&tmp, &max, 1, Type(value), MPI_MAX, rootId, getCommunicator());
    return max;
}
template<typename T>
T MessagePasser::ParallelMin(T value, int rootId) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T tmp = value;
    T min = value;
    MPI_Reduce(&tmp, &min, 1, Type(value), MPI_MIN, rootId, getCommunicator());
    return min;
}
template<typename T>
T MessagePasser::ParallelMin(T value) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T tmp = value;
    T min;
    MPI_Allreduce(&tmp, &min, 1, Type(value), MPI_MIN, getCommunicator());
    return min;
}

template<typename T>
T MessagePasser::ParallelMax(T value) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T tmp = value;
    T max;
    MPI_Allreduce(&tmp, &max, 1, Type(value), MPI_MAX, getCommunicator());
    return max;
}

template<typename T>
std::vector<T> MessagePasser::ParallelMax(const std::vector<T>& vec, int rootId) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    std::vector<T> result;
    result.resize(vec.size());
    if (vec.size() > 0)
        MPI_Reduce((void*) vec.data(), result.data(),
                   bigToInt(vec.size()), Type(T()), MPI_MAX, rootId, getCommunicator());
    return result;
}

template<typename T>
std::vector<T> MessagePasser::ParallelMin(const std::vector<T>& vec, int rootId) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    std::vector<T> result;
    result.resize(vec.size());
    if (vec.size() > 0)
        MPI_Reduce((void*) vec.data(), result.data(),
                   bigToInt(vec.size()), Type(T()), MPI_MIN, rootId, getCommunicator());
    return result;
}

template<typename T>
T MessagePasser::ParallelRankOfMax(T value) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    struct {
      T value;
      int rank;
    } tmp, max;
    tmp.value = value;
    tmp.rank  = Rank();
    MPI_Reduce(&tmp, &max, 1, Type(tmp.value,tmp.rank), MPI_MAXLOC, 0, getCommunicator());
    Broadcast(max.rank, 0);
    return max.rank;
}

inline bool MessagePasser::ParallelOr(bool b) const {
    auto b_int = int(b);
    auto s = ParallelSum(b_int);
    return s != 0;
}

inline bool MessagePasser::ParallelAnd(bool b) const {
    auto b_int = int(b);
    auto s = ParallelSum(b_int);
    return s == NumberOfProcesses();
}

template <typename T>
std::set<T> MessagePasser::ParallelUnion(const std::set<T>& in) const {
    auto all_sets = Gather(in, 0);
    std::set<T> output;
    for(const auto& set : all_sets){
        for(const auto& s : set){
            output.insert(s);
        }
    }
    Broadcast(output, 0);
    return output;
}

template<typename T>
std::complex<T> MessagePasser::ParallelSum(const std::complex<T>& value) const {
    T real_sum = ParallelSum(value.real(), 0);
    T imag_sum = ParallelSum(value.imag(), 0);
    Broadcast(real_sum, 0);
    Broadcast(imag_sum, 0);
    return std::complex<T>{T{real_sum}, T{imag_sum}};
}

template<typename T>
T MessagePasser::ParallelSum(const T& value) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    auto sum = ParallelSum(value, 0);
    Broadcast(sum, 0);
    return sum;
}

template<typename T>
T MessagePasser::ParallelAverage(const T& value) const {
    auto sum = ParallelSum(value);
    return sum * (1.0 / (T)NumberOfProcesses());
}

template<typename T>
std::vector<T> MessagePasser::ElementalMax(const std::vector<T>& vec, int root) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    auto result = vec;
    T t = 0;
    std::vector<T> source = vec;
    MPI_Reduce(source.data(),result.data(),bigToInt(source.size()),Type(t),MPI_MAX,root,getCommunicator());
    return result;
}

template<typename T>
std::vector<T> MessagePasser::ElementalMax(const std::vector<T>& vec) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    auto result = ElementalMax(vec, 0);
    Broadcast(result, 0);
    return result;
}



template<typename T,typename Op>
T MessagePasser::Reduce(const T& value,Op op){
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    T result;
    Reduce(&value,&result,1,op);
    return result;
}

template<typename T,typename Op>
std::vector<T> MessagePasser::Reduce(const std::vector<T>& value,Op op){
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::NonBlockingRecv");
    std::vector<T> result;
    result.resize(value.size());
    Reduce(value.data(),result.data(),bigToInt(value.size()),op);
    return result;
}

template<typename Op>
class MessagePasserReduxOperator{
public:
    MessagePasserReduxOperator(Op binary_op):op(binary_op){}
    template<typename T>
    T apply(T a,T b){
        return op(a,b);
    }
private:
    Op op;
};

static void* MessagePasserReduxOperatorPtr;

template<typename T,typename Op>
void MessagePasser::Reduce(const T* input,T* output,int count,Op op){
    MessagePasserReduxOperator<decltype(op)> redux_operator(op);
    MessagePasserReduxOperatorPtr = &redux_operator;
    auto f = [](void* a_ptr,void* b_ptr,int* len,MPI_Datatype* datatype)->void{
        T* a = (T*)a_ptr;
        T* b = (T*)b_ptr;
        MessagePasserReduxOperator<Op>* user_op;
        user_op = (decltype(user_op))MessagePasserReduxOperatorPtr;
        for(int i=0;i<*len;i++){
            b[i] = user_op->apply(a[i],b[i]);
        }
    };
    void (*fun)(void*,void*,int*,MPI_Datatype*) = f;
    MPI_Op mpi_op;
    MPI_Op_create(fun,0,&mpi_op);
    auto type = Type(T());
    MPI_Allreduce(input,output,count,type,mpi_op,communicator);
    freeIfCustomType(type);
    MPI_Op_free(&mpi_op);
    MessagePasserReduxOperatorPtr = nullptr;
}

template <typename Id, typename Summable, typename GetOwner>
std::map<Id, Summable> MessagePasser::SumAtId(const std::map<Id, Summable>& stuff_to_sum, GetOwner get_owner) const {
    struct SummableAndId{
        Summable summable;
        Id id;
    };
    std::map<int, std::vector<SummableAndId>> send_data_to_owning_ranks;
    for(auto& pair : stuff_to_sum){
        auto id = pair.first;
        auto owner = get_owner(id);
        send_data_to_owning_ranks[owner].push_back({pair.second, id});
    }

    auto incoming_data_to_sum = Exchange(send_data_to_owning_ranks);
    std::map<Id, Summable> data_im_summing;
    for(auto& pair : incoming_data_to_sum) {
        for (auto &summable_and_id : pair.second) {
            auto id = summable_and_id.id;
            auto &summable = summable_and_id.summable;
            if (data_im_summing.count(id) == 0) {
                data_im_summing[id] = summable;
            } else {
                data_im_summing[id] += summable;
            }
        }
    }

    std::map<int, std::vector<SummableAndId>> summed_data_for_return;
    for(const auto& incoming_pair : incoming_data_to_sum){
        auto rank = incoming_pair.first;
        for(const auto& summ_and_id : incoming_pair.second){
            auto id = summ_and_id.id;
            summed_data_for_return[rank].push_back({data_im_summing.at(id), id});
        }
    }

    auto returned_data = Exchange(summed_data_for_return);

    std::map<Id, Summable> output;
    for(auto& returned_pair : returned_data){
        for(auto& summ_and_id : returned_pair.second){
            output[summ_and_id.id] = summ_and_id.summable;
        }
    }
    return output;
}
