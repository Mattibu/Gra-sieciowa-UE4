#include "BufferPool.h"
#include "SpaceLog.h"

#include <mutex>

spacemma::BufferPool::BufferPool(size_t maxSize) : maxSize(maxSize)
{
    SPACEMMA_DEBUG("Initialized BufferPool of size {}!", maxSize);
}

spacemma::BufferPool::~BufferPool()
{
    std::lock_guard lock(mutex);
    SPACEMMA_DEBUG("Clearing buffer pool. {} UB,{} FB,{} US,{} FS,{} TS", usedBuffers.size(), freeBuffers.size(),
                   usedSize, getFreeSize(), byteSize);
    if (usedBuffers.size())
    {
        SPACEMMA_WARN("The buffer pool contains buffers that are currently in use!");
    }
    for (ByteBuffer* buff : usedBuffers)
    {
        freeBuffer(buff);
    }
    flushUnused();
}

spacemma::ByteBuffer* spacemma::BufferPool::getBuffer(size_t size)
{
    std::lock_guard lock(mutex);
    for (std::vector<ByteBuffer*>::iterator it = freeBuffers.begin(); it != freeBuffers.end(); ++it)
    {
        if ((*it)->getTotalSize() < size)
        {
            continue;
        }
        ByteBuffer* buff = *it;
        buff->setUsedSize(size);
        freeBuffers.erase(it);
        addUsed(buff);
        SPACEMMA_TRACE("Reusing buffer {:x} of size {} for desired size {}.", reinterpret_cast<uintptr_t>(buff),
                       buff->getTotalByteSize(), size);
        return buff;
    }
    ByteBuffer* buff = addBuffer(size, false);
    if (!buff)
    {
        return nullptr;
    }
    addUsed(buff);
    return buff;
}

bool spacemma::BufferPool::freeBuffer(gsl::not_null<ByteBuffer*> buffer)
{
    return freeBuffer(buffer, false);
}

bool spacemma::BufferPool::freeAndRemoveBuffer(gsl::not_null<ByteBuffer*> buffer)
{
    return freeBuffer(buffer, true);
}

void spacemma::BufferPool::flushUnused()
{
    std::lock_guard lock(mutex);
    for (std::vector<ByteBuffer*>::iterator it = freeBuffers.begin(); it != freeBuffers.end(); ++it)
    {
        deleteBuffer(*it, false);
    }
    freeBuffers.clear();
}

size_t spacemma::BufferPool::getTotalSize() const
{
    std::lock_guard lock(mutex);
    return byteSize;
}

size_t spacemma::BufferPool::getUsedSize() const
{
    std::lock_guard lock(mutex);
    return usedSize;
}

size_t spacemma::BufferPool::getFreeSize() const
{
    std::lock_guard lock(mutex);
    return byteSize - usedSize;
}

size_t spacemma::BufferPool::getUnallocatedSize() const
{
    std::lock_guard lock(mutex);
    return maxSize - byteSize;
}

size_t spacemma::BufferPool::getMaxSize() const
{
    return maxSize;
}

bool spacemma::BufferPool::freeBuffer(gsl::not_null<ByteBuffer*> buffer, bool remove)
{
    std::lock_guard lock(mutex);
    SPACEMMA_TRACE("Freeing buffer {:x} of size {} ({}).", reinterpret_cast<uintptr_t>(buffer),
                   buffer->getTotalByteSize(), remove);
    std::vector<ByteBuffer*>::iterator it = std::find(usedBuffers.begin(), usedBuffers.end(), buffer);
    if (it == usedBuffers.end())
    {
        return false;
    }
    usedBuffers.erase(it);
    usedSize -= buffer->getTotalSize();
    if (remove)
    {
        deleteBuffer(buffer, false);
    } else
    {
        addToFreeBuffers(buffer);
    }
    return true;
}

void spacemma::BufferPool::addUsed(gsl::not_null<ByteBuffer*> buffer)
{
    std::lock_guard lock(mutex);
    usedBuffers.push_back(buffer);
    usedSize += buffer->getTotalSize();
}

spacemma::ByteBuffer* spacemma::BufferPool::addBuffer(size_t size, bool setFree)
{
    std::lock_guard lock(mutex);
    if (byteSize + size <= maxSize)
    {
        ByteBuffer* buff = new ByteBuffer(size);
        SPACEMMA_TRACE("Adding new buffer {:x} of size {} ({})", reinterpret_cast<uintptr_t>(buff), size, setFree);
        byteSize += size;
        if (setFree)
        {
            addToFreeBuffers(buff);
        }
        return buff;
    }
    SPACEMMA_ERROR("Attempting to overflow the buffer pool by adding {} bytes (the total size is {}, max size is {})",
                   size, byteSize, maxSize);
    return nullptr;
}

void spacemma::BufferPool::addToFreeBuffers(gsl::not_null<ByteBuffer*> buffer)
{
    std::lock_guard lock(mutex);
    if (freeBuffers.empty() || freeBuffers[0]->getTotalSize() >= buffer->getTotalSize())
    {
        freeBuffers.push_back(buffer);
    } else
    {
        for (std::vector<ByteBuffer*>::iterator it = freeBuffers.begin(); it != freeBuffers.end(); ++it)
        {
            if ((*it)->getTotalSize() <= buffer->getTotalSize())
            {
                freeBuffers.insert(it + 1, buffer);
                break;
            }
        }
    }
}

bool spacemma::BufferPool::deleteBuffer(gsl::not_null<ByteBuffer*> buffer, bool checkIfFree)
{
    std::lock_guard lock(mutex);
    SPACEMMA_TRACE("Deleting buffer {:x} of size {} ({})", reinterpret_cast<uintptr_t>(buffer),
                   buffer->getTotalByteSize(), checkIfFree);
    if (checkIfFree)
    {
        std::vector<ByteBuffer*>::iterator it = std::find(freeBuffers.begin(), freeBuffers.end(), buffer);
        if (it == freeBuffers.end())
        {
            SPACEMMA_WARN("Failed to remove buffer {:x} as it's being used!", reinterpret_cast<uintptr_t>(buffer.get()));
            return false;
        }
        freeBuffers.erase(it);
    }
    byteSize -= buffer->getTotalSize();
    delete buffer.get();
    return true;
}
