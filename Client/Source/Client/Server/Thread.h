#pragma once


#include <gsl/gsl-lite.hpp>

#define THREAD_JOIN_TIMEOUT 10000

namespace spacemma
{
    class Thread;
    typedef void (*ThreadFunc)(gsl::not_null<Thread*>, void* ptr);

    /**
     * Base for thread implementations.
     */
    class Thread
    {
    public:
        Thread() = default;
        virtual ~Thread() = default;
        /**
         * Starts the thread with a given ThreadFunc and pointer argument.
         * Returns true if the thread was successfully started, false otherwise.
         */
        virtual bool run(ThreadFunc func, void* ptr) = 0;
        /**
         * Returns true if the thread is currently running, false otherwise.
         */
        virtual bool isRunning() = 0;
        /**
         * Sets the interrupt flag of the thread.
         */
        virtual void interrupt() = 0;
        /**
         * Returns true if the thread interrupt flag is set, false otherwise.
         */
        virtual bool isInterrupted() = 0;
        /**
         * Attempts to join the thread. This call will block further instructions until the thread is joined.
         */
        virtual bool join() = 0;
        /**
         * Terminates the thread.
         */
        virtual bool terminate() = 0;
        /**
         * Returns the ThreadFunc used by this thread.
         */
        ThreadFunc getThreadFunc() const;
        /**
         * Returns the pointer argument of this thread.
         */
        void* getPtr() const;
    protected:
        ThreadFunc threadFunc{ nullptr };
        void* ptr{ nullptr };
    };
}
