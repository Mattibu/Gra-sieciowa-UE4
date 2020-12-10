#pragma once

#include "TCPServer.h"
#include "WinsockUtil.h"

namespace spacemma
{
    struct ClientData
    {
        SOCKET socket{ INVALID_SOCKET };
        unsigned short port{ 0U };
        char recvBuffer[TCP_SOCKET_BUFFER_SIZE]{};
    };

    /**
     * A multi-client TCP server implementation.
     * TODO: remove race conditions
     */
    class WinTCPMultiClientServer final : public TCPServer
    {
    public:
        WinTCPMultiClientServer(BufferPool& bufferPool, unsigned char maxClients);
        ~WinTCPMultiClientServer();
        WinTCPMultiClientServer(WinTCPMultiClientServer&) = delete;
        WinTCPMultiClientServer(WinTCPMultiClientServer&&) = delete;
        WinTCPMultiClientServer& operator=(WinTCPMultiClientServer&) = delete;
        WinTCPMultiClientServer& operator=(WinTCPMultiClientServer&&) = delete;
        bool bindAndListen(gsl::cstring_span ipAddress, unsigned short port) override;
        bool isListening() override;
        bool acceptClient() override;
        unsigned char getClientCount() const;
        std::vector<unsigned short> getClientPorts() const;
        bool isClientAlive(unsigned short clientPort) const;
        bool send(gsl::not_null<ByteBuffer*> buff) override;
        bool send(gsl::not_null<ByteBuffer*> buff, unsigned short clientPort);
        ByteBuffer* receive() override;
        ByteBuffer* receive(unsigned short clientPort);
        bool shutdown() override;
        bool close() override;
        bool isConnected() override;
        bool shutdownClient(unsigned short port) const;
        bool closeClient(unsigned short port) const;
    private:
        bool shutdownServer() const;
        bool closeServer();
        inline ClientData* getClientData(unsigned short port) const;
        SOCKET serverSocket{ INVALID_SOCKET };
        sockaddr_in address{};
        unsigned char maxClients{ 0 };
        gsl::span<ClientData> clientData{};
    };
}
