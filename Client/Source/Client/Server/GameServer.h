// Fill out your copyright notice in the Description page of Project Settings.

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
    template<typename T>
    static T* reinterpretPacket(gsl::not_null<spacemma::ByteBuffer*> packet);
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
};

template<typename T>
T* AGameServer::reinterpretPacket(gsl::not_null<spacemma::ByteBuffer*> packet)
{
    if (packet->getUsedSize() != sizeof(T))
    {
        SERVER_ERROR("Invalid packet {} size ({} != {})!",
                     *packet->getPointer(), sizeof(T), packet->getUsedSize());
        return nullptr;
    }
    return reinterpret_cast<T*>(packet->getPointer());
}
