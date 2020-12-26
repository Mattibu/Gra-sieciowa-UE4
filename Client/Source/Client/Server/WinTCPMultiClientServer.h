#pragma once

#include "TCPMultiClientServer.h"
#include "WinsockUtil.h"

namespace spacemma
{
    struct ClientData
    {
        SOCKET socket{ INVALID_SOCKET };
        unsigned short port{ 0U };
        char recvBuffer[TCP_SOCKET_BUFFER_SIZE]{};
        bool isConnected{};
    };

    /**
     * A multi-client TCP server implementation.
     * TODO: remove race conditions
     */
    class WinTCPMultiClientServer final : public TCPMultiClientServer
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
        unsigned short acceptClient() override;
        unsigned char getClientCount() const override;
        std::vector<unsigned short> getClientPorts() const override;
        bool isClientAlive(unsigned short clientPort) const override;
        bool send(gsl::not_null<ByteBuffer*> buff) override;
        bool sendTo(gsl::not_null<ByteBuffer*> buff, unsigned short clientPort) override;
        ByteBuffer* receive() override;
        ByteBuffer* receiveFrom(unsigned short clientPort) override;
        bool shutdown() override;
        bool close() override;
        bool isConnected() override;
        bool shutdownClient(unsigned short port) const override;
        bool closeClient(unsigned short port) const override;
        bool isConnected(unsigned short port) const override;
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
