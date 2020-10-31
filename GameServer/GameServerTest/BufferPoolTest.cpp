#include "pch.h"
#include "BufferPool.h"

using namespace spacemma;
TEST(BufferPool, SizesProperlyAdjusted)
{
    BufferPool pool(1024UL);
    ASSERT_EQ(0UL, pool.getUsedSize());
    ASSERT_EQ(0UL, pool.getTotalSize());
    ASSERT_EQ(0UL, pool.getFreeSize());
    ASSERT_EQ(1024UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
    ByteBuffer* buff1 = pool.getBuffer(10UL);
    ASSERT_EQ(10UL, buff1->getTotalByteSize());
    ASSERT_EQ(10UL, pool.getUsedSize());
    ASSERT_EQ(10UL, pool.getTotalSize());
    ASSERT_EQ(0UL, pool.getFreeSize());
    ASSERT_EQ(1014UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
    ByteBuffer* buff2 = pool.getBuffer(15UL);
    ASSERT_EQ(15UL, buff2->getTotalByteSize());
    ASSERT_EQ(25UL, pool.getUsedSize());
    ASSERT_EQ(25UL, pool.getTotalSize());
    ASSERT_EQ(0UL, pool.getFreeSize());
    ASSERT_EQ(999UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
    pool.freeBuffer(buff1);
    ASSERT_EQ(15UL, pool.getUsedSize());
    ASSERT_EQ(25UL, pool.getTotalSize());
    ASSERT_EQ(10UL, pool.getFreeSize());
    ASSERT_EQ(999UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
    pool.freeAndRemoveBuffer(buff2);
    ASSERT_EQ(0UL, pool.getUsedSize());
    ASSERT_EQ(10UL, pool.getTotalSize());
    ASSERT_EQ(10UL, pool.getFreeSize());
    ASSERT_EQ(1014UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
    pool.flushUnused();
    ASSERT_EQ(0UL, pool.getUsedSize());
    ASSERT_EQ(0UL, pool.getTotalSize());
    ASSERT_EQ(0UL, pool.getFreeSize());
    ASSERT_EQ(1024UL, pool.getUnallocatedSize());
    ASSERT_EQ(1024UL, pool.getMaxSize());
}