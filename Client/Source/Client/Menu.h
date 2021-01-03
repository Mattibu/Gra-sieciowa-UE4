#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Client/Server/GameServer.h"
#include "Client/GameClient.h"
#include "Menu.generated.h"

UCLASS()
class CLIENT_API UMenu : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = InputValidation)
        bool IsIpValid(FString trimmedIp);
    UFUNCTION(BlueprintCallable, Category = InputValidation)
        bool IsPortValid(FString trimmedPort);
    UFUNCTION(BlueprintCallable, Category = ButtonFunctionality)
        void StartServer(FString trimmedIp, FString trimmedPort, FString map);
    UFUNCTION(BlueprintCallable, Category = ButtonFunctionality)
        void StartClient(FString trimmedNickname, FString trimmedIp, FString trimmedPort, FString map);
};
