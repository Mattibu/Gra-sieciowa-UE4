#pragma once

#include "TCPSocket.h"

namespace spacemma
{
    class TCPServer : public TCPSocket
    {
    public:
        TCPServer(BufferPool& bufferPool);
        ~TCPServer() = default;
        /**
         * Attempts to bind a socket to given IP address a port and start listening.
         * Returns true if the socket was successfully created and is listening, false otherwise.
         */
        virtual bool bindAndListen(gsl::cstring_span ipAddress, unsigned short port) = 0;
        /**
         * Returns true if this socket contains a valid descriptor and is in listening mode, false otherwise.
         */
        virtual bool isListening() = 0;
        /**
         * Attempts to accept a pending client connection. This is a blocking call.
         * Returns true if a client connection was established, false otherwise.
         */
        virtual bool acceptClient() = 0;
    };
}
