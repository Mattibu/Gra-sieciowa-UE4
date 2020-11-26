#include "Thread.h"

spacemma::ThreadFunc spacemma::Thread::getThreadFunc() const
{
    return threadFunc;
}

void* spacemma::Thread::getPtr() const
{
    return ptr;
}
