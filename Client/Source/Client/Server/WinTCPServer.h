#pragma once

#include "TCPServer.h"
#include "WinsockUtil.h"

namespace spacemma
{
    class WinTCPServer final : public TCPServer
    {
    public:
        WinTCPServer(BufferPool& bufferPool);
        ~WinTCPServer();
        WinTCPServer(WinTCPServer&) = delete;
        WinTCPServer(WinTCPServer&&) = delete;
        WinTCPServer& operator=(WinTCPServer&) = delete;
        WinTCPServer& operator=(WinTCPServer&&) = delete;
        bool bindAndListen(gsl::cstring_span ipAddress, unsigned short port) override;
        bool isListening() override;
        bool acceptClient() override;
        bool send(gsl::not_null<ByteBuffer*> buff) override;
        ByteBuffer* receive() override;
        bool shutdown() override;
        bool close() override;
        bool isConnected() override;
    private:
        bool shutdownServer() const;
        bool shutdownClient() const;
        bool closeServer();
        bool closeClient();
        SOCKET serverSocket{INVALID_SOCKET}, clientSocket{INVALID_SOCKET};
        sockaddr_in address{};
        char recvBuffer[TCP_SOCKET_BUFFER_SIZE]{0};
    };
}
