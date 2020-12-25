#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/WinTCPMultiClientServer.h"
#include "Client/Server/Thread.h"
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
        APawn* LocalPlayer = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 ServerPort = 4444;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MaxClients = 8;
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
    static void threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* server);
    static void threadHandleDisconnects(gsl::not_null<spacemma::Thread*> thread, void* server);
    template<typename T>
    void sendPacketToAll(T packet);
    void sendToAll(gsl::not_null<spacemma::ByteBuffer*> buffer);
    template<typename T>
    void sendPacketTo(unsigned short client, T packet);
    void sendTo(unsigned short client, gsl::not_null<spacemma::ByteBuffer*> buffer);
    void disconnectClient(unsigned short client);
    void processPacket(unsigned short sourceClient, gsl::not_null<spacemma::ByteBuffer*> buffer);
    bool isClientAvailable(unsigned short client) const;
    std::recursive_mutex connectionMutex{};
    std::mutex receiveMutex{}, startStopMutex{}, disconnectMutex{}, liveClientsMutex{};
    std::unique_ptr<spacemma::BufferPool> bufferPool;
    std::unique_ptr<spacemma::WinTCPMultiClientServer> tcpServer{};
    spacemma::Thread* acceptThread{}, * processPacketsThread{}, * handleDisconnectsThread{};
    std::vector<std::pair<unsigned short, spacemma::ByteBuffer*>> receivedPackets{};
    std::map<unsigned short, spacemma::Thread*> sendThreads{};
    std::map<unsigned short, spacemma::Thread*> receiveThreads{};
    std::map<unsigned short, APawn*> players{};
    std::map<unsigned short, spacemma::ClientBuffers*> perClientSendBuffers{};
    std::set<unsigned short> disconnectingPlayers{};
    std::set<unsigned short> liveClients{};
};

template<typename T>
void AGameServer::sendPacketToAll(T packet)
{
    for (const auto& pair : perClientSendBuffers)
    {
        sendPacketTo(pair.first, packet);
    }
}

template<typename T>
void AGameServer::sendPacketTo(unsigned short client, T packet)
{
    spacemma::ByteBuffer* buff = bufferPool->getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    sendTo(client, buff);
}
