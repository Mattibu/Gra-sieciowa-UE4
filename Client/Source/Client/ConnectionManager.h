// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DebugLogger.h"
#include "Sockets.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Common/TcpSocketBuilder.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include <vector>
#include <unordered_map>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConnectionManager.generated.h"

UCLASS()
class CLIENT_API AConnectionManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AConnectionManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Test)
	FString IpAddress;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Test)
	int32 Port;
private:
	FSocket *socket;
	FSocket* ListenSocket;
	FSocket* ListenSocket2;
	void connectToServer(FString ipAddress, int socketNumber);
	void createServer(FString ipAddress, int32 socketNumber);
	void receiveData();
	void sendData();
	UFUNCTION(BlueprintCallable, Category = "Tests")
	FVector getPosition(int id);
	std::unordered_map<int, FVector> enemyPositions;

};