#include "ServerLog.h"
#include "BufferPool.h"
#include "WinTCPMultiClientServer.h"
#include "WinTCPServer.h"
#include "GameManager.h";

using namespace spacemma;
std::mutex gameManagerMutex;

void listenForClients(WinTCPMultiClientServer& server, GameManager& gameManager)
{   if (!server.acceptClient())
        printf("ERROR: cannot accept!\n");
    else
    {
        auto clientPorts = server.getClientPorts();
        unsigned short clientPort = clientPorts[clientPorts.size() -1];
        gameManagerMutex.lock();
        gameManager.addEmptyPlayer(clientPort);
        gameManagerMutex.unlock();
        printf("Accepted from port: %hi\n", clientPort);
    }
    listenForClients(server, gameManager);
}


int main()
{
    const unsigned char MAX_CLIENTS = 8U;
    BufferPool bufferPool(1024 * 1024);
    //WinTCPServer server(bufferPool);
    WinTCPMultiClientServer server(bufferPool, MAX_CLIENTS);
    GameManager gameManager;
    if (!server.bindAndListen("127.0.0.1", 4444))
    {
        return 1;
    }
    if (!server.isListening())
    {
        return 2;
    }
    int playerId = 0;
    std::thread t1(listenForClients, std::ref(server), std::ref(gameManager));
    std::vector<unsigned short> ports;
    ByteBuffer* dataBuffer = new ByteBuffer(500);
    while (true)
    {        

        Sleep(2000);
        ports = server.getClientPorts();
        for (auto port : ports)
        {
            if (gameManagerMutex.try_lock())
            {

                int playerId = gameManager.getPlayerIdByPort(port);
                for (int i = 0; i < gameManager.getPlayersCount(); i++)
                {
                    if (i != playerId)
                    {
                        printf("Sent data about player %i to player: %i!\n", i, playerId);
                        uint8_t* data = dataBuffer->getPointer();
                        data[0] = playerId;
                        memcpy(data + 1, gameManager.getPlayerData(i), sizeof(Player));
                        dataBuffer->setUsedSize(sizeof(Player));
                        server.send(dataBuffer, port);
                        Sleep(20);
                    }
                }
                gameManagerMutex.unlock();
            }
            else
            {
                printf("WARNING: Unable to send data - gameManager mutex is locked!");

            }
            
        }
       
    }
    delete dataBuffer;
   // t1.join();

    return 0;
}


