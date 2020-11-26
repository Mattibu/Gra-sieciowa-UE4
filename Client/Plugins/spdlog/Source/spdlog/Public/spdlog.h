// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FspdlogModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    void StartupModule() override;
    void ShutdownModule() override;
};
