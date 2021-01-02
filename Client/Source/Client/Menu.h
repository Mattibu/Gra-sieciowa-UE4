#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
};
