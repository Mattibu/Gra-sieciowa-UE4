#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Client/Server/WinTCPClient.h"
#include "Client/Server/Thread.h"
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
        bool closeConnection();
private:
    static void threadConnect(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadReceive(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadSend(gsl::not_null<spacemma::Thread*> thread, void* client);
    static void threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* client);
    void send(gsl::not_null<spacemma::ByteBuffer*> packet);
    void processPacket(spacemma::ByteBuffer* buffer);
    spacemma::Thread* sendThread{}, * receiveThread{}, * connectThread{}, * processPacketsThread{};
    std::mutex connectionMutex{}, receiveMutex{}, sendMutex{};
    std::vector<spacemma::ByteBuffer*> receivedPackets{}, toSendPackets{};
    spacemma::BufferPool bufferPool{ 1024 * 1024 * 1024 };
    spacemma::WinTCPClient tcpClient{ bufferPool };
};
