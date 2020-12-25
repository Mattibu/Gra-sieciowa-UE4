// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;

using UnrealBuildTool;

public class Client : ModuleRules {
    public Client(ReadOnlyTargetRules Target) : base(Target) {
        bEnableExceptions = true;
        PCHUsage = PCHUsageMode.NoSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking" });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "spdlog", "include"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "ThirdParty", "spdlog", "lib", "spdlog.lib"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "gsl-lite", "include"));

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
