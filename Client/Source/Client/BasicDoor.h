#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasicDoor.generated.h"

UCLASS()
class CLIENT_API ABasicDoor : public AActor
{
    GENERATED_BODY()
public:
    ABasicDoor();
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Door_Parameters)
        FVector OpenOffset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Door_Parameters)
        float SlideTime = 1.0f;
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        bool IsOpen();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        float GetOpenProgress();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        void OpenDoor();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        void CloseDoor();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        void OpenInstantly();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
        void CloseInstantly();
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
    void SetOpenInstantly(bool open);
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
    void SetOpen(bool open);
    UFUNCTION(BlueprintCallable, Category = Door_Movement)
    void SetState(bool open, bool instantly);
protected:
    virtual void BeginPlay() override;
private:
    bool isOpen{ false };
    float openProgress{ 0.0f };
    FVector closedLocation{};
};
