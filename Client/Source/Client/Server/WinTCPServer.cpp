#include "WinTCPServer.h"
#include "SpaceLog.h"

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
        SPACEMMA_ERROR("Provided IP address is empty!");
        return false;
    }
    SPACEMMA_DEBUG("Attempting to bind a socket to {}:{}...", ipAddress.cbegin(), port);
    if (!WinsockUtil::wsaStartup(this))
    {
        return false;
    }
    shutdown();
    close();
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        SPACEMMA_ERROR("Failed to create socket ({})!", WSAGetLastError());
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    address = { AF_INET, htons(port), 0, {0} };
    if (int ret = InetPtonA(address.sin_family, ipAddress.cbegin(), &address.sin_addr); ret != 1)
    {
        if (ret == 0)
        {
            SPACEMMA_ERROR("An invalid IP address was provided!");
        } else
        {
            SPACEMMA_ERROR("Failed to convert the IP address ({})!", WSAGetLastError());
        }
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) != 0)
    {
        SPACEMMA_ERROR("Failed to bind socket ({})!", WSAGetLastError());
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        SPACEMMA_ERROR("Failed to start listening ({})!", WSAGetLastError());
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    char buff[64]{ 0 };
    PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
    if (str == nullptr)
    {
        SPACEMMA_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
        SPACEMMA_DEBUG("Socket bound to and listening!");
    } else
    {
        SPACEMMA_DEBUG("Socket bound to {}:{} and listening!", str, address.sin_port);
    }
    return true;
}

bool spacemma::WinTCPServer::isListening()
{
    return serverSocket != INVALID_SOCKET;
}

unsigned short spacemma::WinTCPServer::acceptClient()
{
    clientSocket = accept(serverSocket, static_cast<sockaddr*>(nullptr), nullptr);
    if (clientSocket == INVALID_SOCKET)
    {
        SPACEMMA_ERROR("Failed to accept a client connection ({})!", WSAGetLastError());
        return 0;
    }
    const int TIMEOUT = TCP_SOCKET_TIMEOUT;
    //if (int ret = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
    //                         reinterpret_cast<const char*>(&TIMEOUT), sizeof(TIMEOUT)); ret == SOCKET_ERROR)
    //{
    //    SPACEMMA_ERROR("Failed to set client receive timeout ({})!", WSAGetLastError());
    //    closesocket(clientSocket);
    //    clientSocket = INVALID_SOCKET;
    //    return 0;
    //}
    if (int ret = setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO,
                             reinterpret_cast<const char*>(&TIMEOUT), sizeof(TIMEOUT)); ret == SOCKET_ERROR)
    {
        SPACEMMA_ERROR("Failed to set client send timeout ({})!", WSAGetLastError());
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return 0;
    }
    sockaddr_in addr;
    int addrSize{ sizeof(addr) };
    if (getpeername(clientSocket, reinterpret_cast<sockaddr*>(&addr), &addrSize) == SOCKET_ERROR)
    {
        SPACEMMA_WARN("Accepted connection, but getpeername failed ({})!", WSAGetLastError());
    } else
    {
        char buff[64]{ 0 };
        PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
        if (str == nullptr)
        {
            SPACEMMA_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
            SPACEMMA_DEBUG("Accepted connection!");
        } else
        {
            SPACEMMA_DEBUG("Accepted connection from {}:{}!", str, addr.sin_port);
        }
    }
    closeServer();
    connected = true;
    return addr.sin_port;
}

bool spacemma::WinTCPServer::send(gsl::not_null<ByteBuffer*> buff)
{
    if (::send(clientSocket, reinterpret_cast<char*>(buff->getPointer()), static_cast<int>(buff->getUsedSize()),
               0) == SOCKET_ERROR)
    {
        const int error = WSAGetLastError();
        switch (error)
        {
            case WSAECONNRESET:
                SPACEMMA_ERROR("Failed to send {} data! Connection reset by peer.", buff->getUsedSize());
                connected = false;
                break;
            case WSAETIMEDOUT:
                SPACEMMA_ERROR("Failed to send {} data! Connection timed out.", buff->getUsedSize());
                connected = false;
                break;
            default:
                SPACEMMA_ERROR("Failed to send {} data ({})!", buff->getUsedSize(), error);
                break;
        }
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
        const int error = WSAGetLastError();
        switch (error)
        {
            case WSAECONNRESET:
                SPACEMMA_ERROR("Failed to receive data! Connection reset by peer.");
                connected = false;
                break;
            case WSAETIMEDOUT:
                SPACEMMA_ERROR("Failed to receive data! Connection timed out.");
                connected = false;
                break;
            default:
                SPACEMMA_ERROR("Failed to receive data ({})!", error);
                break;
        }
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
    return clientSocket != INVALID_SOCKET && connected;
}

bool spacemma::WinTCPServer::shutdownServer() const
{
    if (serverSocket != INVALID_SOCKET)
    {
        if (::shutdown(serverSocket, SD_BOTH) == SOCKET_ERROR)
        {
            SPACEMMA_WARN("Failed to shut down server socket ({})!", WSAGetLastError());
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
            SPACEMMA_WARN("Failed to shut down client socket ({})!", WSAGetLastError());
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
            SPACEMMA_WARN("Failed to close server socket ({})!", WSAGetLastError());
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
            SPACEMMA_WARN("Failed to close client socket ({})!", WSAGetLastError());
            return false;
        }
        clientSocket = INVALID_SOCKET;
    }
    return true;
}
