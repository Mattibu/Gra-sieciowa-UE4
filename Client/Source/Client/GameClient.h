#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/Thread.h"
#include "Client/Server/TCPClient.h"
#include "Client/ShooterPlayer.h"
#include <map>
#include "GameClient.generated.h"

UCLASS()
class CLIENT_API AGameClient : public AActor
{
    struct RecentPosData
    {
        FVector prevPos{};
        FVector nextPos{};
        FVector prevVelocity{};
        FVector nextVelocity{};
        float timePassed{};
    };

    GENERATED_BODY()

public:
    AGameClient();
    virtual void Tick(float DeltaTime) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Data)
        AShooterPlayer* ClientPawn;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        int32 ServerPort = 4444;
    UPROPERTY(EditDefaultsOnly, Category = Spawn_Parameters)
        TSubclassOf<AActor> PlayerBP;
    UPROPERTY(EditDefaultsOnly, Category = Movement_Parameters)
        float InterpolationStrength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = Movement_Parameters)
        float InterpolationSharpness = 0.1f;
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
        void shoot();
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void attachRope(FVector attachPosition);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void detachRope();
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void updateVelocity(FVector velocity);
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void updateRotation();
private:
    static void threadConnect(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* client);
    template<typename T>
    void sendPacket(T packet);
    void send(gsl::not_null<spacemma::ByteBuffer*> packet);
    void processPacket(spacemma::ByteBuffer* buffer);
    void processPendingPacket();
    void interpolateMovement(float deltaTime);
    spacemma::Thread* sendThread{}, * receiveThread{}, * connectThread{};
    std::mutex connectionMutex{}, receiveMutex{}, sendMutex{};
    std::vector<spacemma::ByteBuffer*> receivedPackets{}, toSendPackets{};
    std::unique_ptr<spacemma::BufferPool> bufferPool{};
    std::unique_ptr<spacemma::TCPClient> tcpClient{};
    std::map<unsigned short, AShooterPlayer*> otherPlayers{};
    std::map<unsigned short, RecentPosData> recentPosData{};
    unsigned short playerId{ 0 };
};

template <typename T>
void AGameClient::sendPacket(T packet)
{
    spacemma::ByteBuffer* buff = bufferPool->getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    send(buff);
}
