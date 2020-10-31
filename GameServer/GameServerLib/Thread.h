#pragma once


#include <gsl/gsl-lite.hpp>

#define THREAD_JOIN_TIMEOUT 10000

namespace spacemma
{
    class Thread;
    typedef void (*ThreadFunc)(gsl::not_null<Thread*>, void* ptr);

    class Thread
    {
    public:
        Thread() = default;
        virtual ~Thread() = default;
        virtual bool run(ThreadFunc func, void* ptr) = 0;
        virtual bool isRunning() = 0;
        virtual void interrupt() = 0;
        virtual bool isInterrupted() = 0;
        virtual bool join() = 0;
        virtual bool terminate() = 0;
        ThreadFunc getThreadFunc() const;
        void* getPtr() const;
    protected:
        ThreadFunc threadFunc{nullptr};
        void* ptr{nullptr};
    };
}
