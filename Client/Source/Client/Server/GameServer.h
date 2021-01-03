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
    /**
    * Virtual method which is called once per game loop to update object state.
    */
    virtual void Tick(float DeltaTime) override;
    /**
    * Specifies server ip address.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    /**
    * Specifies server port.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 ServerPort = 4444;
    /**
    * Specifies max number of clients which can be connected to sever.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MaxClients = 8;
    /**
    * Specifies the movement update frequency.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MovementUpdateTickRate = 10;
    UPROPERTY(EditDefaultsOnly, Category = Spawn_Parameters)
        TSubclassOf<AActor> PlayerBP;
    /**
    * Starts the server with resource allocation.
    */
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool startServer();
    /**
    * Checks if server is still running.
    */
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool isServerRunning() const;
    /**
    * Stops server with cleanup.
    */
    UFUNCTION(BlueprintCallable, Category = Server_Management)
        bool stopServer();
protected:
    /**
    * Virtual method which is called once on start.
    */
    virtual void BeginPlay() override;
    /**
    * Virtual method which is called once on end.
    */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    //todo: switch naming convention to match UE4!
    static void threadAcceptClients(gsl::not_null<spacemma::Thread*> thread, void* server);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* args);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* args);
    /**
    * Sends packet to all connected clients.
    */
    template<typename T>
    void sendPacketToAll(T packet);
    /**
    * Sends buffer to all connected clients.
    */
    void sendToAll(gsl::not_null<spacemma::ByteBuffer*> buffer);
    /**
    * Sends packet to all connected clients but one ignored client for whom the packet will not be sent, is specified.
    */
    template<typename T>
    void sendPacketToAllBut(T packet, unsigned short ignoredClient);
    /**
    * Sends buffer to all connected clients but one ignored client for whom the packet will not be sent, is specified.
    */
    void sendToAllBut(gsl::not_null<spacemma::ByteBuffer*> buffer, unsigned short ignoredClient);
    /**
    * Sends packet to all connected clients but two ignored clients for whom the packet will not be sent, are specified.
    */
    template<typename T>
    void sendPacketToAllBut(T packet, unsigned short ignoredClient1, unsigned short ignoredClient2);
    /**
    * Sends buffer to all connected clients but two ignored clients for whom the packet will not be sent, are specified.
    */
    void sendToAllBut(gsl::not_null<spacemma::ByteBuffer*> buffer, unsigned short ignoredClient1, unsigned short ignoredClient2);
    /**
    * Sends packet to client specified by its id.
    */
    template<typename T>
    void sendPacketTo(unsigned short client, T packet);
    /**
    * Sends buffer to client specified by its id.
    */
    void sendTo(unsigned short client, gsl::not_null<spacemma::ByteBuffer*> buffer);
    /**
    * Disconnects client from server.
    */
    void disconnectClient(unsigned short client);
    /**
    * Processes packet received from client.
    */
    void processPacket(unsigned short sourceClient, gsl::not_null<spacemma::ByteBuffer*> buffer);
    /**
    * Handles a player, who is awaiting to be spawned.
    */
    void handlePlayerAwaitingSpawn();
    /**
    * Handles disconnected clients.
    */
    void handlePendingDisconnect();
    /**
    * Checks if there is any available packet and then if it is true, processes the packet.
    */
    void processPendingPacket();
    /**
    * Processes all available packets.
    */
    void processAllPendingPackets();
    /**
    * Send the information about movement of specified client to all clients.
    */
    void broadcastPlayerMovement(unsigned short client);
    /**
    * Send the information about any movement to all clients.
    */
    void broadcastMovingPlayers();
    /**
    * Checks if client is available.
    */
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
