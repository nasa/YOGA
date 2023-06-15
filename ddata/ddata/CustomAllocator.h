#pragma once
#include <cstdlib>
#include <new>
#include <limits>

namespace Linearize {
struct Chunk {
    Chunk* next;
};

class PoolAllocator {
  public:
    PoolAllocator(size_t chunksPerBlock) : mChunksPerBlock(chunksPerBlock) {}

    void* allocate(size_t size) {
        if (mAlloc == nullptr) mAlloc = allocateBlock(size);
        Chunk* freeChunk = mAlloc;
        mAlloc = mAlloc->next;
        return freeChunk;
    }

    void deallocate(void* chunk, size_t size) {
        reinterpret_cast<Chunk*>(chunk)->next = mAlloc;
        mAlloc = reinterpret_cast<Chunk*>(chunk);
    }

  private:
    size_t mChunksPerBlock;

    Chunk* mAlloc = nullptr;

    Chunk* allocateBlock(size_t chunkSize) const {
        auto blockBegin = reinterpret_cast<Chunk*>(std::malloc(mChunksPerBlock * chunkSize));
        Chunk* chunk = blockBegin;

        for (size_t i = 0; i < mChunksPerBlock - 1; ++i) {
            chunk->next = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunkSize);
            chunk = chunk->next;
        }

        chunk->next = nullptr;

        return blockBegin;
    }
};

template <class T>
struct MemoryPool {
    typedef T value_type;

    MemoryPool() = default;
    template <class U>
    constexpr MemoryPool(const MemoryPool<U>&) noexcept {}

    PoolAllocator& getAllocator() {
        static PoolAllocator allocator{128};
        return allocator;
    }

    T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) throw std::bad_array_new_length();
        if (auto p = static_cast<T*>(getAllocator().allocate(n * sizeof(T)))) return p;
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept { getAllocator().deallocate(p, n); }
};

template <class T, class U>
bool operator==(const MemoryPool<T>&, const MemoryPool<U>&) {
    return true;
}
template <class T, class U>
bool operator!=(const MemoryPool<T>&, const MemoryPool<U>&) {
    return false;
}
}