#include "WinTCPMultiClientServer.h"
#include "ServerLog.h"

spacemma::WinTCPMultiClientServer::WinTCPMultiClientServer(BufferPool& bufferPool, unsigned char maxClients)
    : TCPServer(bufferPool), maxClients(maxClients)
{
    if (!maxClients)
    {
        SERVER_ERROR("Max client amount must be greater than zero!");
        //throw std::exception("Max client amount must be greater than zero!");
    }
    clientData = gsl::make_span<ClientData>(new ClientData[maxClients]{}, maxClients);
}

spacemma::WinTCPMultiClientServer::~WinTCPMultiClientServer()
{
    if (isListening() || isConnected())
    {
        shutdown();
        close();
    }
    WinsockUtil::wsaCleanup(this);
    delete[] clientData.begin();
}

bool spacemma::WinTCPMultiClientServer::bindAndListen(gsl::cstring_span ipAddress, unsigned short port)
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
    address = { AF_INET, htons(port), 0, {0} };
    if (int ret = InetPtonA(address.sin_family, ipAddress.cbegin(), &address.sin_addr); ret != 1)
    {
        if (ret == 0)
        {
            SERVER_ERROR("An invalid IP address was provided!");
        } else
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
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        SERVER_ERROR("Failed to start listening ({})!", WSAGetLastError());
        shutdownServer();
        closeServer();
        WinsockUtil::wsaCleanup(this);
        return false;
    }
    char buff[64]{ 0 };
    PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
    if (str == nullptr)
    {
        SERVER_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
        SERVER_DEBUG("Socket bound to and listening!");
    } else
    {
        SERVER_DEBUG("Socket bound to {}:{} and listening!", str, address.sin_port);
    }
    return true;
}

bool spacemma::WinTCPMultiClientServer::isListening()
{
    return serverSocket != INVALID_SOCKET;
}

unsigned short spacemma::WinTCPMultiClientServer::acceptClient()
{
    ClientData* data = getClientData(0U);
    if (data)
    {
        SOCKET clientSocket = accept(serverSocket, static_cast<sockaddr*>(nullptr), nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            SERVER_ERROR("Failed to accept a client connection ({})!", WSAGetLastError());
            return 0;
        }
        sockaddr_in addr;
        int addrSize{ sizeof(addr) };
        if (getpeername(clientSocket, reinterpret_cast<sockaddr*>(&addr), &addrSize) == SOCKET_ERROR)
        {
            SERVER_WARN("Accepted connection, but getpeername failed ({})!", WSAGetLastError());
            SERVER_ERROR("Unable to acquire the client port. Closing connection!");
            closesocket(clientSocket);
            return 0;
        }
        char buff[64]{ 0 };
        PCSTR str = InetNtopA(AF_INET, &address.sin_addr, buff, 64);
        if (str == nullptr)
        {
            SERVER_WARN("Unable to convert the address to string ({})!", WSAGetLastError());
            SERVER_DEBUG("Accepted connection!");
        } else
        {
            SERVER_DEBUG("Accepted connection from {}:{}!", str, addr.sin_port);
        }
        data->port = addr.sin_port;
        data->socket = clientSocket;
        return addr.sin_port;
    }
    return 0;
}

unsigned char spacemma::WinTCPMultiClientServer::getClientCount() const
{
    unsigned char result{ 0U };
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port)
        {
            ++result;
        }
    }
    return result;
}

std::vector<unsigned short> spacemma::WinTCPMultiClientServer::getClientPorts() const
{
    std::vector<unsigned short> result{};
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port)
        {
            result.push_back(clientData[i].port);
        }
    }
    return result;
}

bool spacemma::WinTCPMultiClientServer::isClientAlive(unsigned short clientPort) const
{
    return getClientData(clientPort) != nullptr;
}

bool spacemma::WinTCPMultiClientServer::send(gsl::not_null<ByteBuffer*> buff)
{
    bool sent = false;
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port)
        {
            if (::send(clientData[i].socket, reinterpret_cast<char*>(buff->getPointer()), static_cast<int>(buff->getUsedSize()),
                       0) == SOCKET_ERROR)
            {
                SERVER_ERROR("Failed to send {} data ({}) to client at port {}!", buff->getUsedSize(), WSAGetLastError(), clientData[i].port);
            } else
            {
                sent = true;
            }
        }
    }
    return sent;
}

bool spacemma::WinTCPMultiClientServer::send(gsl::not_null<ByteBuffer*> buff, unsigned short port)
{
    ClientData* data = getClientData(port);
    if (data)
    {
        if (::send(data->socket, reinterpret_cast<char*>(buff->getPointer()), static_cast<int>(buff->getUsedSize()),
                   0) == SOCKET_ERROR)
        {
            SERVER_ERROR("Failed to send {} data ({})!", buff->getUsedSize(), WSAGetLastError());
            return false;
        }
        return true;
    }
    return false;
}

spacemma::ByteBuffer* spacemma::WinTCPMultiClientServer::receive()
{
    return nullptr; // unspecified client to receive from, just return nullptr
}

spacemma::ByteBuffer* spacemma::WinTCPMultiClientServer::receive(unsigned short port)
{
    ClientData* data = getClientData(port);
    if (data)
    {
        int received = recv(data->socket, data->recvBuffer, TCP_SOCKET_BUFFER_SIZE, 0);
        if (received == 0)
        {
            return nullptr;
        }
        if (received == SOCKET_ERROR)
        {
            SERVER_ERROR("Failed to receive data ({}) from client at port {}!", WSAGetLastError(), data->port);
            return nullptr;
        }
        ByteBuffer* buff = bufferPool->getBuffer(received);
        memcpy(buff->getPointer(), data->recvBuffer, received);
        return buff;
    }
    return nullptr;
}

bool spacemma::WinTCPMultiClientServer::shutdown()
{
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port)
        {
            shutdownClient(clientData[i].port);
        }
    }
    return shutdownServer();
}

bool spacemma::WinTCPMultiClientServer::close()
{
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port)
        {
            closeClient(clientData[i].port);
        }
    }
    return closeServer();
}

bool spacemma::WinTCPMultiClientServer::isConnected()
{
    return getClientCount() != 0;
}

bool spacemma::WinTCPMultiClientServer::shutdownServer() const
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

bool spacemma::WinTCPMultiClientServer::shutdownClient(unsigned short port) const
{
    if (!port)
    {
        return false;
    }
    ClientData* data = getClientData(port);
    if (data)
    {
        if (::shutdown(data->socket, SD_BOTH) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to shut down client socket ({})!", WSAGetLastError());
            return false;
        }
        return true;
    }
    return false;
}

bool spacemma::WinTCPMultiClientServer::closeServer()
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

bool spacemma::WinTCPMultiClientServer::closeClient(unsigned short port) const
{
    if (!port)
    {
        return false;
    }
    ClientData* data = getClientData(port);
    if (data)
    {
        if (closesocket(data->socket) == SOCKET_ERROR)
        {
            SERVER_WARN("Failed to close client socket ({})!", WSAGetLastError());
            return false;
        }
        data->port = 0;
        data->socket = INVALID_SOCKET;
        return true;
    }
    return false;
}

spacemma::ClientData* spacemma::WinTCPMultiClientServer::getClientData(unsigned short port) const
{
    for (unsigned char i = 0; i < maxClients; ++i)
    {
        if (clientData[i].port == port)
        {
            return &clientData[i];
        }
    }
    return nullptr;
}
