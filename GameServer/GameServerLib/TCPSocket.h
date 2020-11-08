#pragma once

#include "BufferPool.h"

#define TCP_SOCKET_BUFFER_SIZE 16384

namespace spacemma
{
    /**
     * Base for TCP socket connections.
     */
    class TCPSocket
    {
    public:
        TCPSocket(BufferPool& bufferPool);
        virtual ~TCPSocket() = default;
        /**
         * Sends data through this socket. This is a blocking call.
         * Returns true if data was successfully sent, false otherwise.
         */
        virtual bool send(gsl::not_null<ByteBuffer*> buff) = 0;
        /**
         * Attempts to receive data acquired by this socket. This is a blocking call.
         * Returns a valid ByteBuffer pointer containing received data or null pointer if
         * failed or there is no data to be received.
         */
        virtual ByteBuffer* receive() = 0;
        /**
         * Shuts down the socket. This will unlock any pending send/receive attempts with an error code.
         * Returns true if the socket is already shut down or was shutdown successfully, false otherwise.
         */
        virtual bool shutdown() = 0;
        /**
         * Closes the socket descriptor.
         * Returns true if the socket is already closed or was closed successfully, false otherwise.
         */
        virtual bool close() = 0;
        /**
         * Returns true if this socket contains a valid descriptor, false otherwise.
         */
        virtual bool isConnected() = 0;
    protected:
        BufferPool* bufferPool{ nullptr };
    };
}
