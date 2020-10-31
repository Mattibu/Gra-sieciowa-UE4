#pragma once

#include "TCPClient.h"
#include "WinsockUtil.h"

namespace spacemma
{
    class WinTCPClient final : public TCPClient
    {
    public:
        WinTCPClient(BufferPool& bufferPool);
        ~WinTCPClient();
        WinTCPClient(WinTCPClient&) = delete;
        WinTCPClient(WinTCPClient&&) = delete;
        WinTCPClient& operator=(WinTCPClient&) = delete;
        WinTCPClient& operator=(WinTCPClient&&) = delete;
        bool connect(gsl::cstring_span ipAddress, unsigned short port) override;
        bool send(gsl::not_null<ByteBuffer*> buff) override;
        ByteBuffer* receive() override;
        bool shutdown() override;
        bool close() override;
        bool isConnected() override;
    private:
        SOCKET socket{INVALID_SOCKET};
        sockaddr_in address{};
        char recvBuffer[TCP_SOCKET_BUFFER_SIZE]{0};
    };
}
