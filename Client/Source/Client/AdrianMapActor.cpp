#include "AdrianMapActor.h"

AAdrianMapActor::AAdrianMapActor()
{
    PrimaryActorTick.bCanEverTick = true;

}

void AAdrianMapActor::BeginPlay()
{
    Super::BeginPlay();
    switchTimer = SwitchInterval;
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
    switchTimer += DeltaTime;
    if (switchTimer >= SwitchInterval)
    {
        switchTimer -= SwitchInterval;
        switchCycled = !switchCycled;
        updateDoors(switchCycled);
    }
}
