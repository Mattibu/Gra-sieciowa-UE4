#pragma once
#include "Buffer.h"

#include <mutex>
#include <vector>

namespace spacemma
{
    /**
     * A buffer pool used for efficient buffer recycling.
     */
    class BufferPool final
    {
    public:
        BufferPool(size_t maxSize);
        ~BufferPool();
        BufferPool(BufferPool&) = delete;
        BufferPool(BufferPool&&) = delete;
        BufferPool& operator=(BufferPool&) = delete;
        BufferPool& operator=(BufferPool&&) = delete;
        /**
         * Returns a buffer for which the total size is greater or equal to size provided with an argument.
         * If the buffer pool does not contain an appropriate buffer, the buffer is created, added to the pool and returned.
         */
        ByteBuffer* getBuffer(size_t size);
        /**
         * Marks the given buffer as free so that it can be used later.
         * The provided buffer must be owned by this pool.
         */
        bool freeBuffer(gsl::not_null<ByteBuffer*> buffer);
        /**
         * Marks the given buffer as free, deallocates it and removes it from the pool.
         */
        bool freeAndRemoveBuffer(gsl::not_null<ByteBuffer*> buffer);
        /**
         * Removes and deallocates all unused buffers in the pool.
         */
        void flushUnused();
        /**
         * Returns the total size of all contained buffers.
         */
        size_t getTotalSize() const;
        /**
         * Returns the total size of all buffers that are currently in use.
         */
        size_t getUsedSize() const;
        /**
         * Returns the total size of all buffers that are currently unused.
         */
        size_t getFreeSize() const;
        /**
         * Returns the difference between maximum size of this pool and it's current size.
         */
        size_t getUnallocatedSize() const;
        /**
         * Returns the maximum combined buffer size that can be contained in this pool.
         */
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
        size_t byteSize{ 0UL };
        size_t usedSize{ 0UL };
    };
}
