#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstring>

class Message {
public:
    Message() = default;

    Message(const std::vector<char> &v) : read_offset(0), blob(v) {}

    char* data() {return blob.data();}
    const char* data() const {return blob.data();}
    size_t size() const {return blob.size();}
    void resize(size_t n) {blob.resize(n);}
    void reserve(size_t n) {blob.reserve(n);}
    void finalize() { read_offset = 0; }

    template<typename T>
    Message(T t) {
        pack(t);
    }

    template<typename T, typename Packer>
    Message(T t, Packer packer) {
        packer(*this, t);
    }

    inline void pack(const std::string& s){
        size_t n = s.size();
        pack(n);
        size_t write_offset = blob.size();
        blob.resize(blob.size()+n);
        std::copy(s.begin(),s.end(),blob.data()+write_offset);
    }

    inline void pack(const char* string_literal){
        std::string s(string_literal);
        pack(s);
    }

    inline void pack(const void* ptr, size_t bytes){
        size_t write_offset = blob.size();
        blob.resize(blob.size() + bytes);
        memcpy(blob.data() + write_offset, (char*)ptr, bytes);
    }

    void pack(const Message& other_msg){
        size_t bytes = other_msg.size();
        pack(bytes);
        size_t write_offset = blob.size();
        blob.resize(blob.size() + bytes);
        memcpy(blob.data() + write_offset, other_msg.data(), bytes);
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    pack(const T &x) {
        size_t bytes = sizeof(T);
        size_t write_offset = blob.size();
        blob.resize(blob.size() + bytes);
        memcpy(blob.data() + write_offset, &x, bytes);
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    pack(const std::vector <T> &vec) {
        int n = bigToInt(vec.size());
        pack(n);
        size_t bytes = n * sizeof(T);
        size_t write_offset = blob.size();
        blob.resize(blob.size() + bytes);
        memcpy(blob.data() + write_offset, vec.data(), bytes);
    }

    template<typename T, typename Packer>
    void packVector(const std::vector <T> &vec,  Packer packer) {
        int n = bigToInt(vec.size());
        pack(n);
        for(int i = 0; i < n; i++)
            packer(*this, vec[i]);
    }

    inline void pack(const std::vector<bool> &vec) {
        int n = bigToInt(vec.size());
        pack(n);
        size_t write_offset = blob.size();
        blob.resize(blob.size() + vec.size());
        for (int i = 0; i < n; i++) blob[write_offset++] = char(vec[i]);
    }

    template<typename T>
    inline void pack(const std::set<T>& a){
        std::vector<T> vec(a.begin(),a.end());
        pack(vec);
    }

    inline void unpack(std::string& s){
        size_t n = 0;
        unpack(n);
        s.resize(n);
        copyInto(n,&s[0]);
    }

    template<typename T>
    inline void unpack(std::set<T>& a){
        std::vector<T> vec;
        unpack(vec);
        for(auto& element:vec)
            a.insert(element);
    }

    template<typename T, typename Y>
    void pack(const std::map <T, Y> &map) {
        int n = bigToInt(map.size());
        pack(n);
        for (auto &pair : map) {
            pack(pair.first);
            pack(pair.second);
        }
    }

    void unpack(Message& msg){
        size_t bytes = 0;
        unpack(bytes);
        msg.resize(bytes);
        copyInto(bytes,msg.data());
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    unpack(T &x) {
        copyInto(1,&x);
    }

    template<typename T, typename UnPacker>
    void unpack(T &x, UnPacker unPacker) {
        unPacker(*this, x);
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    unpack(std::vector <T> &vec) {
        int n = 0;
        unpack(n);
        vec.resize(n);
        copyInto(n,&vec[0]);
    }

    template<typename T, typename Unpacker>
    void unpackVector(std::vector <T> &vec, Unpacker unpacker) {
        int n = 0;
        unpack(n);
        vec.resize(n);
        for(int i = 0; i < n; i++){
            unpacker(*this, vec[i]);
        }
    }

    inline void unpack(std::vector<bool> &vec) {
        int n = 0;
        unpack(n);
        vec.resize(n);
        for (int i = 0; i < n; i++) vec[i] = bool(blob[read_offset++]);
    }

    template<typename T, typename Y>
    void unpack(std::map <T, Y> &map) {
        int n = 0;
        unpack(n);
        for (int i = 0; i < n; i++) {
            T key{};
            unpack(key);
            map[key] = {};
            unpack(map[key]);
        }
    }

private:
    size_t read_offset = 0;
    std::vector<char> blob;
    void throwIfRangeExceeded(size_t bytes_to_copy){
        if(read_offset+bytes_to_copy > blob.size())
            throw std::domain_error("MessagePasser::Message reading past end of data");
    }

    template<typename T>
    void copyInto(size_t n,T* ptr){
        size_t bytes = n* sizeof(T);
        throwIfRangeExceeded(bytes);
        const T* begin = (const T*)(blob.data()+read_offset);
        std::copy(begin,begin+n,ptr);
        read_offset += bytes;
    }
};

