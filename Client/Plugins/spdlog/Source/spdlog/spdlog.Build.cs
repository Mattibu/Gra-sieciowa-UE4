// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;

using UnrealBuildTool;

public class spdlog : ModuleRules
{
    public spdlog(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        // Type = ModuleType.External;
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "thirdparty", "spdlog", "include"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "thirdparty", "gsl-lite", "include"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "thirdparty", "spdlog", "lib", "spdlog.lib"));

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "Projects",
                "Sockets",
                "Networking"
            }
        );
    }
}
