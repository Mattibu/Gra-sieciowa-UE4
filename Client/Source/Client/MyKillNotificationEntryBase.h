#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyKillNotificationEntryBase.generated.h"

UCLASS()
class CLIENT_API UMyKillNotificationEntryBase : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent, Category = PlayerNames)
        void SetPlayerNames(const FString& killerName, const FString& victimName);
};
