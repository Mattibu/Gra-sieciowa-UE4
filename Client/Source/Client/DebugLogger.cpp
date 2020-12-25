// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugLogger.h"
#include "Engine/Engine.h"

DebugLogger::DebugLogger()
{
}

DebugLogger::~DebugLogger()
{
}

void DebugLogger::logError(FString message)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, message);
}
void DebugLogger::logWarning(FString message)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, message);

}
void DebugLogger::logInfo(FString message)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, message);
}
