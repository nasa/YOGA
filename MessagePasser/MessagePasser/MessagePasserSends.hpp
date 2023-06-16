
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

template<typename T>
void MessagePasser::Send(const std::vector<T>& vec, int length, int destination) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::");
    auto return_type = Type(T());
    MPI_Send(vec.data(), length, return_type, destination, 0, getCommunicator());
    freeIfCustomType(return_type);
}

template<typename T>
void MessagePasser::Send(const std::vector<T>& vec, int destination) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::");
    int length = (int) vec.size();
    Send(vec, length, destination);
}

template<typename T>
void MessagePasser::Send(const std::vector<std::vector<T>>& vec, int destination) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::");
    // pack into a contiguous buffer
    std::vector<T> sendBuffer;
    std::vector<int> sendBufferMap = {0};
    for (auto& row:vec)
        sendBufferMap.push_back(bigToInt(row.size() + sendBufferMap.back()));
    sendBuffer.reserve(sendBufferMap.back());
    for (auto& row:vec)
        for (auto val:row)
            sendBuffer.push_back(val);
    Send(sendBufferMap, destination);
    Send(sendBuffer, destination);
}

template<typename T>
MessagePasser::MessageStatus MessagePasser::NonBlockingSend(const T& value, int destination) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::");
    MessageStatus status;
    auto return_type = Type(value);
    MPI_Isend(&value, 1, return_type, destination, 0, getCommunicator(), status.request());
    freeIfCustomType(return_type);
    return status;
}

template<typename T>
MessagePasser::MessageStatus MessagePasser::NonBlockingSend(const T&& value, int destination) const {
    std::string msg = "MessagePasser::NonBlockingSend: a temporary variable was passed in,\n";
    msg += "it could go out of scope before this function completes.  Send in an L-value.\n";
    throw std::logic_error(msg);
}

inline MessagePasser::Promise MessagePasser::NonBlockingSend(Message&& message,int destination) const {
    Promise promise(std::move(message));
    MPI_Isend(promise.get()->data(), bigToInt(promise.get()->size()), MPI_CHAR, destination, 0, getCommunicator(), promise.request());
    return promise;
}

template<typename T>
MessagePasser::MessageStatus MessagePasser::NonBlockingSend(
        const std::vector<T>& vec,
        int length, int destination) const {
    static_assert(std::is_trivially_copyable<T>::value, "Must be able to trivially copy datatype for MessagePasser::");
    MessageStatus status;
    auto return_type = Type(T());
    MPI_Isend(vec.data(), length, return_type, destination, 0, getCommunicator(), status.request());
    freeIfCustomType(return_type);
    return status;
}

template<typename T>
MessagePasser::MessageStatus MessagePasser::NonBlockingSend(
        const std::vector<T>& vec, int destination) const {
    return NonBlockingSend(vec, bigToInt(vec.size()), destination);
}

inline MessagePasser::MessageStatus MessagePasser::NonBlockingSend(
    const std::string& s, int destination) const {
    MessageStatus status;
    MPI_Isend(s.data(), bigToInt(s.size()), MPI_CHAR, destination, 0, getCommunicator(), status.request());
    return status;
}
