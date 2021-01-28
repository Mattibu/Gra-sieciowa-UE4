#include "HUDBase.h"

void UHUDBase::AddKillNotification(const FString& killerName, const FString& victimName)
{
    UMyKillNotificationEntryBase* notification = CreateWidget<UMyKillNotificationEntryBase>(this, KillNotificationBP);
    notification->SetPlayerNames(killerName, victimName);
    NotificationBox->AddChildToVerticalBox(notification);
    notificationTimes.emplace(notification, NotificationTime);
}

void UHUDBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    for (auto it = notificationTimes.begin(); it != notificationTimes.end();)
    {
        it->second -= InDeltaTime;
        if (it->second <= 0.0f)
        {
            it->first->RemoveFromParent();
            it = notificationTimes.erase(it);
        } else
        {
            ++it;
        }
    }
}
