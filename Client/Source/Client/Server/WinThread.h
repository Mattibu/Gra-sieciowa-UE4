#pragma once
#include "Thread.h"
#include "ServerLog.h"

#include <mutex>

#define THREAD_RESULT_SUCCESS 0UL
#define THREAD_RESULT_TERMINATED 1UL

namespace spacemma
{
    class WinThread final : public Thread
    {
    public:
        WinThread() = default;
        ~WinThread();
        WinThread(WinThread&) = delete;
        WinThread(WinThread&&) = delete;
        WinThread& operator=(WinThread&) = delete;
        WinThread& operator=(WinThread&&) = delete;
        bool run(ThreadFunc func, void* ptr) override;
        bool isRunning() override;
        void interrupt() override;
        bool isInterrupted() override;
        bool join() override;
        bool terminate() override;
        DWORD getThreadId() const;
    private:
        void closeThreadHandle();
        static DWORD threadProc(LPVOID param);
        std::atomic_bool interrupted{false}, running{false};
        std::mutex mutex;
        HANDLE threadHandle{nullptr};
    };
}
