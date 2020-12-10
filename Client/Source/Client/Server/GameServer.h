// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/WinTCPMultiClientServer.h"
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
private:
    static void threadAcceptClients(void* server);
    static void threadSend(void* server, unsigned short clientPort);
    static void threadReceive(void* server, unsigned short clientPort);
    const unsigned char MAX_CLIENTS{ 8 };
    std::atomic_bool serverActive{ false };
    spacemma::BufferPool bufferPool{ 1024 * 1024 * 1024 };
    spacemma::WinTCPMultiClientServer tcpServer{ bufferPool, MAX_CLIENTS };
    std::map<unsigned short, AActor*> players{};
};
