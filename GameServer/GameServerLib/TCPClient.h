#pragma once

#include "TCPSocket.h"

namespace spacemma
{
    class TCPClient : public TCPSocket
    {
    public:
        TCPClient(BufferPool& bufferPool);
        ~TCPClient() = default;
        /**
         * Attempts to connect to the server given by an IP address and port. This is a blocking call.
         * Returns true if the connection is successfully established, false otherwise.
         */
        virtual bool connect(gsl::cstring_span ipAddress, unsigned short port) = 0;
    };
}
