#include "pch.h"
#include "WinThread.h"

using namespace spacemma;

std::mutex mutex{};

void basicThreadTestProc(gsl::not_null<Thread*> thread, void* ptr)
{
    while (!thread->isInterrupted())
    {
        if (mutex.try_lock())
        {
            mutex.unlock();
            break;
        }
    }
}

TEST(WinThread, BasicThreadTest)
{
    WinThread thread{};
    {
        std::lock_guard lock(mutex);
        ASSERT_EQ(0UL, thread.getThreadId());
        ASSERT_FALSE(thread.isRunning());
        ASSERT_TRUE(thread.join());
        ASSERT_TRUE(thread.run(basicThreadTestProc, nullptr));
        ASSERT_NE(0UL, thread.getThreadId());
        ASSERT_TRUE(thread.isRunning());
        thread.interrupt();
        ASSERT_TRUE(thread.isInterrupted());
        Sleep(5);
        ASSERT_FALSE(thread.isRunning());
        ASSERT_TRUE(thread.join());
        ASSERT_EQ(0UL, thread.getThreadId());
        ASSERT_TRUE(thread.run(basicThreadTestProc, nullptr));
        ASSERT_NE(0UL, thread.getThreadId());
        ASSERT_FALSE(thread.isInterrupted());
        ASSERT_TRUE(thread.isRunning());
    }
    Sleep(5);
    ASSERT_FALSE(thread.isRunning());
    ASSERT_TRUE(thread.join());
    ASSERT_EQ(0UL, thread.getThreadId());
}