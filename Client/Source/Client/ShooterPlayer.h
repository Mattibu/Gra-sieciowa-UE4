// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterPlayer.generated.h"

UCLASS()
class CLIENT_API AShooterPlayer : public ACharacter
{
    GENERATED_BODY()

public:
    AShooterPlayer();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        void AttachRope(FVector position, bool broadcast);
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        void SetRopeCooldown(float cooldown, bool broadcast);
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        void DetachRope(bool broadcast);
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        void SetSpeedVector(FVector speed, bool broadcast);
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        void SetRotationVector(FVector rotationVector, bool broadcast);
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        float GetRopeCooldown();
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        FVector GetRotationVector();
    UFUNCTION(BlueprintImplementableEvent, Category = Player_Control)
        FVector GetSpeedVector();
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
};
