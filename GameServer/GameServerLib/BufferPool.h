#pragma once
#include "Buffer.h"

#include <mutex>
#include <vector>

namespace spacemma
{
    class BufferPool final
    {
    public:
        BufferPool(size_t maxSize);
        ~BufferPool();
        BufferPool(BufferPool&) = delete;
        BufferPool(BufferPool&&) = delete;
        BufferPool& operator=(BufferPool&) = delete;
        BufferPool& operator=(BufferPool&&) = delete;
        ByteBuffer* getBuffer(size_t size);
        bool freeBuffer(gsl::not_null<ByteBuffer*> buffer);
        bool freeAndRemoveBuffer(gsl::not_null<ByteBuffer*> buffer);
        void flushUnused();
        size_t getTotalSize() const;
        size_t getUsedSize() const;
        size_t getFreeSize() const;
        size_t getUnallocatedSize() const;
        size_t getMaxSize() const;
    private:
        bool freeBuffer(gsl::not_null<ByteBuffer*> buffer, bool remove);
        void addUsed(gsl::not_null<ByteBuffer*> buffer);
        ByteBuffer* addBuffer(size_t size, bool setFree);
        void addToFreeBuffers(gsl::not_null<ByteBuffer*> buffer);
        bool deleteBuffer(gsl::not_null<ByteBuffer*> buffer, bool checkIfFree);
        mutable std::recursive_mutex mutex{};
        std::vector<ByteBuffer*> usedBuffers{}, freeBuffers{};
        size_t maxSize{};
        size_t byteSize{0UL};
        size_t usedSize{0UL};
    };
}
