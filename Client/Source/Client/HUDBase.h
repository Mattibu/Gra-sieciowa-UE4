#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Client/MyKillNotificationEntryBase.h"
#include "Components/VerticalBox.h"
#include <map>
#include "HUDBase.generated.h"
UCLASS()
class CLIENT_API UHUDBase : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category = Kill_Notifications)
        float NotificationTime = 5.0f;
    UPROPERTY(EditDefaultsOnly, Category = Kill_Notifications)
        TSubclassOf<UMyKillNotificationEntryBase> KillNotificationBP;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Kill_Notifications)
        UVerticalBox* NotificationBox;
    UFUNCTION(BlueprintCallable, Category = Kill_Notifications)
        void AddKillNotification(const FString& killerName, const FString& victimName);
    void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
    std::map<UMyKillNotificationEntryBase*, float> notificationTimes{};
};
