// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class CLIENT_API DebugLogger
{
public:
	DebugLogger();
	~DebugLogger();
	static void logError(FString message);
	static void logWarning(FString message);
	static void logInfo(FString message);
};
