

#include "ShooterPlayer.h"

AShooterPlayer::AShooterPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AShooterPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

void AShooterPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AShooterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

