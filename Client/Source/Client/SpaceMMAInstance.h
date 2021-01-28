#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Client/ShooterPlayer.h"
#include "Client/Server/GameServer.h"
#include "Client/GameClient.h"
#include "SpaceMMAInstance.generated.h"

UCLASS()
class CLIENT_API USpaceMMAInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    enum class LevelInitialization
    {
        None,
        Client,    // client configuration
        Server     // server configuration
    };
    LevelInitialization Initialization{ LevelInitialization::None };
    UPROPERTY(EditDefaultsOnly, Category = GameManagement)
        TSubclassOf<AGameServer> ServerBP;
    UPROPERTY(EditDefaultsOnly, Category = GameManagement)
        TSubclassOf<AGameClient> ClientBP;
    UPROPERTY(EditDefaultsOnly, Category = GameManagement)
        TSubclassOf<AShooterPlayer> PlayerBP;
    UFUNCTION(BlueprintCallable, Category = Initialization)
        void Initialize();
    UFUNCTION(BlueprintCallable, Category = Menu)
        void GoToMenu();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Universal_Parameters)
        FString ServerIpAddress = "127.0.0.1";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Universal_Parameters)
        int32 ServerPort = 4444;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Parameters)
        FString Nickname = "Player";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Parameters)
        bool ForceStartFromMenu = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Client_Parameters)
        FString ForceStartMapName = "";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        int32 MaxClients = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Server_Parameters)
        float RoundTime = 300;
};
