// Copyright Epic Games, Inc. All Rights Reserved.

#include "spdlog.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "spdlogLibrary/ExampleLibrary.h"

#define LOCTEXT_NAMESPACE "FspdlogModule"

void FspdlogModule::StartupModule()
{

}

void FspdlogModule::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FspdlogModule, spdlog)
