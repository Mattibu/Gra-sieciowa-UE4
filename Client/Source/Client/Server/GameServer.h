#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/TCPMultiClientServer.h"
#include "Client/Server/Thread.h"
#include "Client/ShooterPlayer.h"
#include "Engine/World.h"
#include <map>
#include <set>
#include "GameServer.generated.h"

UCLASS()
class CLIENT_API AGameServer : public AActor
{
    GENERATED_BODY()

public:
    AGameServer();
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 ServerPort = 4444;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MaxClients = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MovementUpdateTickRate = 10;
    UPROPERTY(EditDefaultsOnly, Category = Spawn_Parameters)
        TSubclassOf<AActor> PlayerBP;
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool startServer();
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool isServerRunning() const;
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool stopServer();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    //todo: switch naming convention to match UE4!
    static void threadAcceptClients(gsl::not_null<spacemma::Thread*> thread, void* server);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* args);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* args);
    template<typename T>
    void sendPacketToAll(T packet);
    void sendToAll(gsl::not_null<spacemma::ByteBuffer*> buffer);
    template<typename T>
    void sendPacketToAllBut(T packet, unsigned short ignoredClient);
    void sendToAllBut(gsl::not_null<spacemma::ByteBuffer*> buffer, unsigned short ignoredClient);
    template<typename T>
    void sendPacketToAllBut(T packet, unsigned short ignoredClient1, unsigned short ignoredClient2);
    void sendToAllBut(gsl::not_null<spacemma::ByteBuffer*> buffer, unsigned short ignoredClient1, unsigned short ignoredClient2);
    template<typename T>
    void sendPacketTo(unsigned short client, T packet);
    void sendTo(unsigned short client, gsl::not_null<spacemma::ByteBuffer*> buffer);
    void disconnectClient(unsigned short client);
    void processPacket(unsigned short sourceClient, gsl::not_null<spacemma::ByteBuffer*> buffer);
    void handlePlayerAwaitingSpawn();
    void handlePendingDisconnect();
    void processPendingPacket();
    void processAllPendingPackets();
    void broadcastPlayerMovement(unsigned short client);
    void broadcastMovingPlayers();
    bool isClientAvailable(unsigned short client);
    std::recursive_mutex connectionMutex{};
    std::mutex receiveMutex{}, startStopMutex{}, disconnectMutex{}, liveClientsMutex{}, spawnAwaitingMutex{};
    std::unique_ptr<spacemma::BufferPool> bufferPool;
    std::unique_ptr<spacemma::TCPMultiClientServer> tcpServer{};
    spacemma::Thread* acceptThread{};
    std::vector<std::pair<unsigned short, spacemma::ByteBuffer*>> receivedPackets{};
    std::map<unsigned short, spacemma::Thread*> sendThreads{};
    std::map<unsigned short, spacemma::Thread*> receiveThreads{};
    std::map<unsigned short, AShooterPlayer*> players{};
    std::map<unsigned short, spacemma::ClientBuffers*> perClientSendBuffers{};
    std::map<unsigned short, bool> recentlyMoving{}; // todo: replace these 3 maps with 1 map of struct containing bool, FVector and FRotator, current solution is nasty
    std::map<unsigned short, FVector> recentPositions{};
    std::map<unsigned short, FRotator> recentRotations{};
    std::set<unsigned short> disconnectingPlayers{};
    std::set<unsigned short> liveClients{};
    std::set<unsigned short> playersAwaitingSpawn{};
    float movementUpdateDelta{}, currentMovementUpdateDelta{ 0.0f };
};

template<typename T>
void AGameServer::sendPacketToAll(T packet)
{
    for (const std::map<unsigned short, spacemma::ClientBuffers*>::value_type& pair : perClientSendBuffers)
    {
        sendPacketTo(pair.first, packet);
    }
}

template<typename T>
void AGameServer::sendPacketToAllBut(T packet, unsigned short ignoredClient)
{
    for (const std::map<unsigned short, spacemma::ClientBuffers*>::value_type& pair : perClientSendBuffers)
    {
        if (pair.first != ignoredClient)
        {
            sendPacketTo(pair.first, packet);
        }
    }
}

template<typename T>
void AGameServer::sendPacketToAllBut(T packet, unsigned short ignoredClient1, unsigned short ignoredClient2)
{
    for (const std::map<unsigned short, spacemma::ClientBuffers*>::value_type& pair : perClientSendBuffers)
    {
        if (pair.first != ignoredClient1 && pair.first != ignoredClient2)
        {
            sendPacketTo(pair.first, packet);
        }
    }
}

template<typename T>
void AGameServer::sendPacketTo(unsigned short client, T packet)
{
    spacemma::ByteBuffer* buff = bufferPool->getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    sendTo(client, buff);
}
