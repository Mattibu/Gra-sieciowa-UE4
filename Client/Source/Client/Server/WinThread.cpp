#include "WinThread.h"
#include "ServerLog.h"

spacemma::WinThread::~WinThread()
{
    if (threadHandle)
    {
        if (running && !join())
        {
            SERVER_WARN("Failed to join thread while destroying the object, terminating...");
            if (!terminate())
            {
                SERVER_ERROR("Failed to terminate thread while destroying the object!");
            }
        }
        closeThreadHandle();
    }
}

bool spacemma::WinThread::run(ThreadFunc func, void* _ptr)
{
    std::lock_guard lock(mutex);
    if (running)
    {
        SERVER_WARN("Attempted to invoke run on a thread that is already running!");
        return false;
    }
    running = true;
    interrupted = false;
    this->threadFunc = func;
    this->ptr = _ptr;
    if (threadHandle)
    {
        closeThreadHandle();
    }
    threadHandle = CreateThread(nullptr, 0ULL, threadProc, this, 0UL, nullptr);
    if (!threadHandle)
    {
        SERVER_ERROR("Failed to create a thread ({})!", GetLastError());
        running = false;
        return false;
    }
    return true;
}

bool spacemma::WinThread::isRunning()
{
    if (running)
    {
        DWORD exitCode;
        if (!GetExitCodeThread(threadHandle, &exitCode))
        {
            SERVER_WARN("Failed to check thread exit code ({})!", GetLastError());
            return true;
        }
        switch (exitCode)
        {
        case STILL_ACTIVE:
            return true;
        case THREAD_RESULT_TERMINATED:
            SERVER_WARN("Detected thread termination exit code!");
            break;
        case THREAD_RESULT_SUCCESS:
            break;
        default:
            SERVER_WARN("Unrecognized thread exit code {}!", exitCode);
            break;
        }
        running = false;
    }
    return running;
}

void spacemma::WinThread::interrupt()
{
    interrupted = true;
}

bool spacemma::WinThread::isInterrupted()
{
    return interrupted;
}

bool spacemma::WinThread::join()
{
    std::lock_guard lock(mutex);
    if (!running)
    {
        closeThreadHandle();
        return true;
    }
    interrupt();
    DWORD waitResult = WaitForSingleObject(threadHandle, THREAD_JOIN_TIMEOUT);
    switch (waitResult)
    {
    case WAIT_TIMEOUT:
        SERVER_WARN("Thread join attempt timed out!");
        return false;
    case WAIT_FAILED:
        SERVER_ERROR("Thread join attempt failed ({})!", GetLastError());
        running = false;
        return false;
    case WAIT_ABANDONED:
        SERVER_WARN("Thread join attempt resulted in WAIT_ABANDONED!");[[fallthrough]];
    default:
        [[fallthrough]];
    case WAIT_OBJECT_0:
        running = false;
        closeThreadHandle();
        return true;
    }
}

bool spacemma::WinThread::terminate()
{
    std::lock_guard lock(mutex);
    if (!running)
    {
        return true;
    }
    if (!TerminateThread(threadHandle, 1UL))
    {
        SERVER_WARN("Thread termination attempt failed ({})!", GetLastError());
        return false;
    }
    running = false;
    closeThreadHandle();
    return true;
}

DWORD spacemma::WinThread::getThreadId() const
{
    return threadHandle ? GetThreadId(threadHandle) : 0UL;
}

void spacemma::WinThread::closeThreadHandle()
{
    if (threadHandle)
    {
        if (!CloseHandle(threadHandle))
        {
            SERVER_WARN("Failed to close the thread handle!");
        }
        else
        {
            threadHandle = nullptr;
        }
    }
}

DWORD WINAPI spacemma::WinThread::threadProc(LPVOID param)
{
    Thread* thread = reinterpret_cast<Thread*>(param);
    ThreadFunc threadFunc = thread->getThreadFunc();
    if (threadFunc)
    {
        threadFunc(thread, thread->getPtr());
    }
    else
    {
        SERVER_WARN("An empty thread has been started!");
    }
    return THREAD_RESULT_SUCCESS;
}
