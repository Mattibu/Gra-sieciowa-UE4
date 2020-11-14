#include "ServerLog.h"
#include "BufferPool.h"
#include "WinTCPMultiClientServer.h"

using namespace spacemma;

int main()
{
    const unsigned char MAX_CLIENTS = 8U;
    BufferPool bufferPool(1024 * 1024);
    WinTCPMultiClientServer server(bufferPool, MAX_CLIENTS);
    return 0;
}
