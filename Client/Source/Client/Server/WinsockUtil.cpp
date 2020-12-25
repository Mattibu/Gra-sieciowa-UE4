#include "WinsockUtil.h"

std::vector<void*> spacemma::WinsockUtil::wsaInvokers{};

bool spacemma::WinsockUtil::wsaStartup(void* owner)
{
    if (wsaInvokers.empty())
    {
        WSADATA wsaData;
        SPACEMMA_DEBUG("Running WSAStartup...");
        if (int ret = WSAStartup(MAKEWORD(2, 2), &wsaData); ret != 0)
        {
            SPACEMMA_ERROR("WSAStartup failed ({})!", ret);
            return false;
        }
        wsaInvokers.push_back(owner);
    }
    else if (std::find(wsaInvokers.begin(), wsaInvokers.end(), owner) == wsaInvokers.end())
    {
        wsaInvokers.push_back(owner);
    }
    return true;
}

bool spacemma::WinsockUtil::wsaCleanup(void* owner)
{
    std::vector<void*>::iterator it{std::find(wsaInvokers.begin(), wsaInvokers.end(), owner)};
    if (it != wsaInvokers.end())
    {
        wsaInvokers.erase(it);
    }
    if (wsaInvokers.empty())
    {
        SPACEMMA_DEBUG("Running WSACleanup...");
        if (int ret = WSACleanup(); ret != 0)
        {
            SPACEMMA_ERROR("WSACleanup failed ({})!", WSAGetLastError());
            return false;
        }
    }
    return true;
}
