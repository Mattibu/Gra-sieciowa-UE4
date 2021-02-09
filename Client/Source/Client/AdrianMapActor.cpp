#include "AdrianMapActor.h"
#include "Client/Server/SpaceLog.h"

AAdrianMapActor::AAdrianMapActor()
{
    PrimaryActorTick.bCanEverTick = true;

}

void AAdrianMapActor::BeginPlay()
{
    Super::BeginPlay();
    timebase = FDateTime::UtcNow();
}

void AAdrianMapActor::updateDoorState(bool force)
{
    float time = static_cast<float>((FDateTime::UtcNow() - timebase).GetTotalSeconds());
    bool cycled = static_cast<int>(time / SwitchInterval) % 2 == 0;
    if (cycled != switchCycled)
    {
        switchCycled = cycled;
        updateDoors(cycled, force);
    }
}

void AAdrianMapActor::updateDoors(bool cycle, bool force)
{
    for (auto& door : FrontBigDoors)
    {
        door->SetState(!cycle, force);
    }
    for (auto& door : SideBigDoors)
    {
        door->SetState(cycle, force);
    }
    for (auto& door : SmallDoors)
    {
        door->SetState(cycle, force);
    }
}

void AAdrianMapActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    updateDoorState(false);
}

int64 AAdrianMapActor::GetTimestamp()
{
    return timebase.ToUnixTimestamp();
}

void AAdrianMapActor::SetTimestamp(int64 ts)
{
    SetTimebase(FDateTime::FromUnixTimestamp(ts));
}

void AAdrianMapActor::SetTimebase(FDateTime tb)
{
    timebase = tb;
    updateDoorState(true);
}
