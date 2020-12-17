#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/WinTCPMultiClientServer.h"
#include "Client/Server/Thread.h"
#include <map>
#include "GameServer.generated.h"

UCLASS()
class CLIENT_API AGameServer : public AActor
{
    GENERATED_BODY()

public:
    AGameServer();
    virtual void Tick(float DeltaTime) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    static void threadAcceptClients(gsl::not_null<spacemma::Thread*> thread, void* server);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* args);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* args);
    static void threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* server);
    void sendToAll(gsl::not_null<spacemma::ByteBuffer*> buffer);
    void sendTo(unsigned short client, gsl::not_null<spacemma::ByteBuffer*> buffer);
    void processPacket(unsigned short sourceClient, gsl::not_null<spacemma::ByteBuffer*> buffer);
    const unsigned char MAX_CLIENTS{ 8 };
    std::atomic_bool serverActive{ false };
    std::recursive_mutex connectionMutex{};
    std::mutex receiveMutex{};
    spacemma::BufferPool bufferPool{ 1024 * 1024 * 1024 };
    spacemma::WinTCPMultiClientServer tcpServer{ bufferPool, MAX_CLIENTS };
    spacemma::Thread* acceptThread{}, * processPacketsThread{};
    std::vector<std::pair<unsigned short, spacemma::ByteBuffer*>> receivedPackets{};
    std::map<unsigned short, spacemma::Thread*> sendThreads{};
    std::map<unsigned short, spacemma::Thread*> receiveThreads{};
    std::map<unsigned short, AActor*> players{};
    std::map<unsigned short, spacemma::ClientBuffers*> perClientSendBuffers{};
    gsl::cstring_span serverIpAddress{ "127.0.0.1" };
    unsigned short serverPort{ 4444 };
};
