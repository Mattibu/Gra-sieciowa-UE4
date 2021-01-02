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
	/**
	* Logs error message on the screen.
	*/
	static void logError(FString message);
	/**
	* Logs warning message on the screen.
	*/
	static void logWarning(FString message);
	/**
	* Logs info message on the screen.
	*/
	static void logInfo(FString message);
};
