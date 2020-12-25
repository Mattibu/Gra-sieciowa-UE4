#pragma once

#include "SpaceLog.h"

#include <WS2tcpip.h>
#include <vector>

namespace spacemma
{
    class WinsockUtil final
    {
    public:
        WinsockUtil() = delete;
        static bool wsaStartup(void* owner);
        static bool wsaCleanup(void* owner);
    private:
        static std::vector<void*> wsaInvokers;
    };
}
