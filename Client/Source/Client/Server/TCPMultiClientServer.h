#pragma once

#include "TCPServer.h"

namespace spacemma
{
    struct ClientBuffers
    {
        std::recursive_mutex bufferMutex{};
        std::vector<ByteBuffer*> buffers{}; // todo: change it to queue
    };

    class TCPMultiClientServer : public TCPServer
    {
    public:
        TCPMultiClientServer(BufferPool& bufferPool) : TCPServer(bufferPool) {}
        virtual unsigned char getClientCount() const = 0;
        virtual std::vector<unsigned short> getClientPorts() const = 0;
        virtual bool isClientAlive(unsigned short clientPort) const = 0;
        virtual bool sendTo(gsl::not_null<ByteBuffer*> buff, unsigned short clientPort) = 0;
        virtual ByteBuffer* receiveFrom(unsigned short clientPort) = 0;
        virtual bool shutdownClient(unsigned short port) const = 0;
        virtual bool closeClient(unsigned short port) const = 0;
        virtual bool isConnected(unsigned short port) const = 0;
    };
}
