#include "WinTCPClient.h"
#include "ServerLog.h"

spacemma::WinTCPClient::WinTCPClient(BufferPool& bufferPool) : TCPClient(bufferPool) {}

spacemma::WinTCPClient::~WinTCPClient()
{
    if (isConnected())
    {
        shutdown();
        close();
    }
    WinsockUtil::wsaCleanup(this);
}

bool spacemma::WinTCPClient::connect(gsl::cstring_span ipAddress, unsigned short port)
{
    if (ipAddress.empty())
    {
        SERVER_ERROR("Provided IP address is empty!");
        return false;
    }
    SERVER_DEBUG("Attempting to connect to {}:{}...", ipAddress.cbegin(), port);
    if (!WinsockUtil::wsaStartup(this))
    {
        return false;
    }
    shutdown();
    close();
    socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
    {
        SERVER_ERROR("Failed to create socket ({})!", WSAGetLastError());
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    address = {AF_INET, port, 0, {0}};
    if (int ret = InetPtonA(address.sin_family, ipAddress.cbegin(), &address.sin_addr); ret != 1)
    {
        if (ret == 0)
        {
            SERVER_ERROR("An invalid IP address was provided!");
        }
        else
        {
            SERVER_ERROR("Failed to convert the IP address ({})!", WSAGetLastError());
        }
        shutdown();
        close();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    if (::connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        SERVER_ERROR("Connection failed ({})!", WSAGetLastError());
        shutdown();
        close();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    SERVER_DEBUG("Connection established!");
    return true;
}

bool spacemma::WinTCPClient::send(gsl::not_null<ByteBuffer*> buff)
{
    if (::send(socket, reinterpret_cast<char*>(buff->getPointer()), static_cast<int>(buff->getUsedSize()), 0) ==
        SOCKET_ERROR)
    {
        SERVER_ERROR("Failed to send {} data ({})!", buff->getUsedSize(), WSAGetLastError());
        return false;
    }
    return true;
}

spacemma::ByteBuffer* spacemma::WinTCPClient::receive()
{
    int received = recv(socket, recvBuffer, TCP_SOCKET_BUFFER_SIZE, 0);
    if (received == 0)
    {
        return nullptr;
    }
    if (received == SOCKET_ERROR)
    {
        SERVER_ERROR("Failed to receive data ({})!", WSAGetLastError());
        return nullptr;
    }
    ByteBuffer* buff = bufferPool->getBuffer(received);
    memcpy(buff->getPointer(), recvBuffer, received);
    return buff;
}

bool spacemma::WinTCPClient::shutdown()
{
    if (socket != INVALID_SOCKET)
    {
        if (::shutdown(socket, SD_BOTH) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to shut down socket ({})!", WSAGetLastError());
            return false;
        }
    }
    return true;
}

bool spacemma::WinTCPClient::close()
{
    if (socket != INVALID_SOCKET)
    {
        if (closesocket(socket) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to close socket ({})!", WSAGetLastError());
            return false;
        }
        socket = INVALID_SOCKET;
    }
    return true;
}

bool spacemma::WinTCPClient::isConnected()
{
    return socket != INVALID_SOCKET;
}
