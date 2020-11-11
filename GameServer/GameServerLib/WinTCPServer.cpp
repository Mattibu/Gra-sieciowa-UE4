#include "WinTCPServer.h"
#include "ServerLog.h"

spacemma::WinTCPServer::WinTCPServer(BufferPool& bufferPool) : TCPServer(bufferPool) {}

spacemma::WinTCPServer::~WinTCPServer()
{
    if (isListening() || isConnected())
    {
        shutdown();
        close();
    }
    WinsockUtil::wsaCleanup(this);
}

bool spacemma::WinTCPServer::bindAndListen(gsl::cstring_span ipAddress, unsigned short port)
{
    if (ipAddress.empty())
    {
        SERVER_ERROR("Provided IP address is empty!");
        return false;
    }
    SERVER_DEBUG("Attempting to bind a socket to {}:{}...", ipAddress.cbegin(), port);
    if (!WinsockUtil::wsaStartup(this))
    {
        return false;
    }
    shutdown();
    close();
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
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
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) != 0)
    {
        SERVER_ERROR("Failed to bind socket ({})!", WSAGetLastError());
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        SERVER_ERROR("Failed to start listening ({})!", WSAGetLastError());
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    char buff[64]{0};
    PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
    if (str == nullptr)
    {
        SERVER_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
        SERVER_DEBUG("Socket bound to and listening!");
    }
    else
    {
        SERVER_DEBUG("Socket bound to {}:{} and listening!", str, address.sin_port);
    }
    return true;
}

bool spacemma::WinTCPServer::isListening()
{
    return serverSocket != INVALID_SOCKET;
}

bool spacemma::WinTCPServer::acceptClient()
{
    clientSocket = accept(serverSocket, static_cast<sockaddr*>(nullptr), nullptr);
    if (clientSocket == INVALID_SOCKET)
    {
        SERVER_ERROR("Failed to accept a client connection ({})!", WSAGetLastError());
        return false;
    }
    sockaddr_in addr;
    int addrSize{sizeof(addr)};
    if (getpeername(clientSocket, reinterpret_cast<sockaddr*>(&addr), &addrSize) == SOCKET_ERROR)
    {
        SERVER_WARN("Accepted connection, but getpeername failed ({})!", WSAGetLastError());
    }
    else
    {
        char buff[64]{0};
        PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
        if (str == nullptr)
        {
            SERVER_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
            SERVER_DEBUG("Accepted connection!");
        }
        else
        {
            SERVER_DEBUG("Accepted connection from {}:{}!", str, addr.sin_port);
        }
    }
    closeServer();
    return true;
}

bool spacemma::WinTCPServer::send(gsl::not_null<ByteBuffer*> buff)
{
    if (::send(clientSocket, reinterpret_cast<char*>(buff->getPointer()), static_cast<int>(buff->getUsedSize()),
               0) == SOCKET_ERROR)
    {
        SERVER_ERROR("Failed to send {} data ({})!", buff->getUsedSize(), WSAGetLastError());
        return false;
    }
    return true;
}

spacemma::ByteBuffer* spacemma::WinTCPServer::receive()
{
    int received = recv(clientSocket, recvBuffer, TCP_SOCKET_BUFFER_SIZE, 0);
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

bool spacemma::WinTCPServer::shutdown()
{
    return shutdownClient() && shutdownServer();
}

bool spacemma::WinTCPServer::close()
{
    return closeClient() && closeServer();
}

bool spacemma::WinTCPServer::isConnected()
{
    return clientSocket != INVALID_SOCKET;
}

bool spacemma::WinTCPServer::shutdownServer() const
{
    if (serverSocket != INVALID_SOCKET)
    {
        if (::shutdown(serverSocket, SD_BOTH) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to shut down server socket ({})!", WSAGetLastError());
            return false;
        }
    }
    return true;
}

bool spacemma::WinTCPServer::shutdownClient() const
{
    if (clientSocket != INVALID_SOCKET)
    {
        if (::shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to shut down client socket ({})!", WSAGetLastError());
            return false;
        }
    }
    return true;
}

bool spacemma::WinTCPServer::closeServer()
{
    if (serverSocket != INVALID_SOCKET)
    {
        if (closesocket(serverSocket) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to close server socket ({})!", WSAGetLastError());
            return false;
        }
        serverSocket = INVALID_SOCKET;
    }
    return true;
}

bool spacemma::WinTCPServer::closeClient()
{
    if (clientSocket != INVALID_SOCKET)
    {
        if (closesocket(clientSocket) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to close client socket ({})!", WSAGetLastError());
            return false;
        }
        clientSocket = INVALID_SOCKET;
    }
    return true;
}
