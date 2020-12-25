#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/WinTCPClient.h"
#include "Client/Server/Thread.h"
#include <map>
#include "GameClient.generated.h"

UCLASS()
class CLIENT_API AGameClient : public AActor
{
    GENERATED_BODY()

public:
    AGameClient();
    virtual void Tick(float DeltaTime) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Data)
        APawn* ClientPawn;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        int32 ServerPort = 4444;
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool startConnecting();
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnecting();
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnected();
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isIdentified() const;
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnectedAndIdentified();
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool closeConnection();
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void shoot(FVector location, FRotator rotator);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void changeSpeed(float speedValue, FVector speedVector);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void rotate(FVector rotationVector);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void attachRope(FVector attachPosition);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void detachRope();
private:
    static void threadConnect(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* client);
    template<typename T>
    void sendPacket(T packet);
    void send(gsl::not_null<spacemma::ByteBuffer*> packet);
    void processPacket(spacemma::ByteBuffer* buffer);
    spacemma::Thread* sendThread{}, * receiveThread{}, * connectThread{}, * processPacketsThread{};
    std::mutex connectionMutex{}, receiveMutex{}, sendMutex{};
    std::vector<spacemma::ByteBuffer*> receivedPackets{}, toSendPackets{};
    spacemma::BufferPool bufferPool{ 1024 * 1024 * 1024 };
    spacemma::WinTCPClient tcpClient{ bufferPool };
    std::map<unsigned short, APawn*> otherPlayers{};
    unsigned short playerId{ 0 };
};

template <typename T>
void AGameClient::sendPacket(T packet)
{
    spacemma::ByteBuffer* buff = bufferPool.getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    send(buff);
}
