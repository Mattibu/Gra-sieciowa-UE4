#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasicDoor.h"
#include <vector>
#include "AdrianMapActor.generated.h"

UCLASS()
class CLIENT_API AAdrianMapActor : public AActor
{
    GENERATED_BODY()
public:
    AAdrianMapActor();
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Doors)
        TArray<ABasicDoor*> FrontBigDoors;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Doors)
        TArray<ABasicDoor*> SideBigDoors;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Doors)
        TArray<ABasicDoor*> SmallDoors;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Doors)
        float SwitchInterval = 8.0f;
    virtual void Tick(float DeltaTime) override;
protected:
    virtual void BeginPlay() override;

private:
    void updateDoors(bool cycle, bool force = false);
    bool switchCycled{ false };
    float switchTimer{ 0.0f };
};