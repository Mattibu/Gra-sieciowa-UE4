#include "GameServer.h"
#include "Client/Server/ServerLog.h"

// Sets default values
AGameServer::AGameServer()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGameServer::BeginPlay()
{
    Super::BeginPlay();
    if (tcpServer.bindAndListen("127.0.0.1", 4444))
    {
        serverActive = true;
        //todo: start all server threads etc.
    } else
    {
        SERVER_ERROR("Game server initialization failed!");
    }
}

void AGameServer::threadAcceptClients(void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    do
    {
        if (srv->players.size() < srv->MAX_CLIENTS)
        {
            //todo: handle accepting clients and for each accepted one start appropriate threads
        }
    } while (srv->serverActive);
}

void AGameServer::threadSend(void* server, unsigned short clientPort)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    //todo: implement per-player sending
}

void AGameServer::threadReceive(void* server, unsigned short clientPort)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    //todo: implement per-player receiving
}

// Called every frame
void AGameServer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

