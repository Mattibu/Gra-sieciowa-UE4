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
    /**
     * Contains information about current and previous location and velocity.
     * Used for movement interpolation.
     */
    struct RecentPosData
    {
        FVector prevPos{};
        FVector nextPos{};
        FVector prevVelocity{};
        FVector nextVelocity{};
        float timePassed{};
    };
    /**
     * Contains all importaant information about a remote player.
     */
    struct OtherPlayerData final
    {
        AShooterPlayer* player{};
        std::string nickname{};
        unsigned int kills{ 0 };
        unsigned int deaths{ 0 };
    };

    GENERATED_BODY()

public:
    AGameClient();
    /**
    * Virtual method which is called once per game loop to update object state.
    */
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Data)
        AShooterPlayer* ClientPawn;
    /**
    * The nickname of this player. If empty, invalid or already occupied, the server will reject the connection.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        FString Nickname;
    /**
    * The ip of a server to which player will try to connect to.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    /**
    * The port of a server to which player will try to connect to.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Connection_Parameters)
        int32 ServerPort = 4444;
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool startConnecting();
    /**
    * Checks if client is trying to establish connection to the server.
    */
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnecting();
    /**
    * Checks if client is connected to the server.
    */
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnected();
    /**
    * Checks if client has valid id.
    */
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isIdentified() const;
    /**
    * Checks if client has valid id and is connected.
    */
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool isConnectedAndIdentified();
    /**
    * Closes the connection to the server.
    */
    UFUNCTION(BlueprintCallable, Category = Connection_Management)
        bool closeConnection();
    /**
    * If client is connected and identified sends packet to the server.
    * Information about shoot.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void shoot(FVector shootingPosition);
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about attachment of the rope.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void attachRope(FVector attachPosition);
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about detachment of the rope.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void detachRope();
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about player's velocity.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void updateVelocity(FVector velocity);
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about player's rotation.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void updateRotation();
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about player's death.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void dead();
    /**
    * If client is connected and identified sends the packet to the server.
    * Information about player's respawn.
    */
    UFUNCTION(BlueprintCallable, Category = Send_Net_Packet)
        void respawn(FVector position, FRotator rotator);
protected:
    /**
    * Virtual method which is called once on start.
    */
    virtual void BeginPlay() override;
    /**
    * Virtual method which is called once on end.
    */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditDefaultsOnly, Category = Spawn_Parameters)
        TSubclassOf<AActor> PlayerBP;
    UPROPERTY(EditDefaultsOnly, Category = Movement_Parameters)
        float InterpolationStrength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = Movement_Parameters)
        float InterpolationSharpness = 0.1f;
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
    std::map<unsigned short, OtherPlayerData> otherPlayers{};
    std::map<unsigned short, RecentPosData> recentPosData{};
    unsigned short playerId{ 0 };
    unsigned int kills{0};
    unsigned int deaths{0};
};

template <typename T>
void AGameClient::sendPacket(T packet)
{
    spacemma::ByteBuffer* buff = bufferPool->getBuffer(sizeof(T));
    memcpy(buff->getPointer(), &packet, sizeof(T));
    send(buff);
}
