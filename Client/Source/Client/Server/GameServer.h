#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/TCPMultiClientServer.h"
#include "Client/Server/Thread.h"
#include "Client/ShooterPlayer.h"
#include "Client/Shared/Packets.h"
#include "Engine/World.h"
#include <map>
#include <set>
#include "GameServer.generated.h"

UCLASS()
class CLIENT_API AGameServer : public AActor
{
    /**
     * Contains all important information about a specific client.
     */
    struct GameClientData final
    {
        spacemma::Thread* sendThread{};
        spacemma::Thread* receiveThread{};
        AShooterPlayer* player{ nullptr };
        spacemma::ClientBuffers* sendBuffers{};
        bool recentlyMoving{};
        FVector recentPosition{};
        FRotator recentRotation{};
        unsigned int kills{};
        unsigned int deaths{};
        std::string nickname{};

    };
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
    /**
    * Specifies the round duration in seconds
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 RoundTime = 600;
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
    * Checks if client is available and in game.
    */
    bool isClientLive(unsigned short client);
    /**
    * Updates round timer and handle round restart
    */
    void handleRoundTimer(float deltaTime);
    std::recursive_mutex connectionMutex{}, liveClientsMutex{};
    std::mutex receiveMutex{}, startStopMutex{}, disconnectMutex{}, spawnAwaitingMutex{}, unverifiedPlayersMutex{};
    std::unique_ptr<spacemma::BufferPool> bufferPool;
    std::unique_ptr<spacemma::TCPMultiClientServer> tcpServer{};
    spacemma::Thread* acceptThread{};
    std::vector<std::pair<unsigned short, spacemma::ByteBuffer*>> receivedPackets{};
    std::set<unsigned short> disconnectingPlayers{};
    std::set<unsigned short> liveClients{};
    std::set<unsigned short> playersAwaitingSpawn{};
    std::set<unsigned short> unverifiedPlayers{};
    std::map<unsigned short, GameClientData> gameClientData{};
    float movementUpdateDelta{}, currentMovementUpdateDelta{ 0.0f };
    std::string mapName{};
    float currentRoundTime{ 0.0f };
};

template<typename T>
void AGameServer::sendPacketToAll(T packet)
{
    static_assert(!std::is_same<spacemma::packets::S2C_CreatePlayer, T>());
    std::lock_guard lock(liveClientsMutex);
    for (unsigned short port : liveClients)
    {
        sendPacketTo(port, packet);
    }
}

template<typename T>
void AGameServer::sendPacketToAllBut(T packet, unsigned short ignoredClient)
{
    static_assert(!std::is_same<spacemma::packets::S2C_CreatePlayer, T>());
    std::lock_guard lock(liveClientsMutex);
    for (unsigned short port : liveClients)
    {
        if (port != ignoredClient)
        {
            sendPacketTo(port, packet);
        }
    }
}

template<typename T>
void AGameServer::sendPacketToAllBut(T packet, unsigned short ignoredClient1, unsigned short ignoredClient2)
{
    static_assert(!std::is_same<spacemma::packets::S2C_CreatePlayer, T>());
    std::lock_guard lock(liveClientsMutex);
    for (unsigned short port : liveClients)
    {
        if (port != ignoredClient1 && port != ignoredClient2)
        {
            sendPacketTo(port, packet);
        }
    }
}

template<typename T>
void AGameServer::sendPacketTo(unsigned short client, T packet)
{
    static_assert(!std::is_same<spacemma::packets::S2C_CreatePlayer, T>());
    spacemma::ByteBuffer* buff = bufferPool->getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    sendTo(client, buff);
}
