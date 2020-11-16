#include "ServerLog.h"
#include "BufferPool.h"
#include "WinTCPMultiClientServer.h"
#include "WinTCPServer.h"

using namespace spacemma;

int main()
{
    const unsigned char MAX_CLIENTS = 8U;
    BufferPool bufferPool(1024 * 1024);
    //WinTCPServer server(bufferPool);
    WinTCPMultiClientServer server(bufferPool, MAX_CLIENTS);
    if (!server.bindAndListen("127.0.0.1", 4444))
    {
        return 1;
    }
    if (!server.isListening())
    {
        return 2;
    }
    if (!server.acceptClient())
    {
        return 3;
    }
    printf("Accepted!\n");
    return 0;
}
