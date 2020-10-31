#pragma once

#include "BufferPool.h"

#define TCP_SOCKET_BUFFER_SIZE 16384

namespace spacemma
{
    class TCPSocket
    {
    public:
        TCPSocket(BufferPool& bufferPool);
        virtual ~TCPSocket() = default;
        virtual bool send(gsl::not_null<ByteBuffer*> buff) = 0; // blocking
        virtual ByteBuffer* receive() = 0; // blocking, might return nullptr
        virtual bool shutdown() = 0; // should stop any send/receive attempts
        virtual bool close() = 0;
        virtual bool isConnected() = 0;
    protected:
        BufferPool* bufferPool{nullptr};
    };
}
