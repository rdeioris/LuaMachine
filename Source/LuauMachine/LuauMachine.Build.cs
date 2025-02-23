// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

using System.IO;
using UnrealBuildTool;

public class LuauMachine : ModuleRules
{
    public LuauMachine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary"),
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary/VM/include"),
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary/Common/include"),
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary/Compiler/include"),
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary/AST/include"),
				Path.Combine(ModuleDirectory, "../ThirdParty/LuauLibrary/VM/src"),
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "HTTP",
                "Json",
                "PakFile"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "InputCore",
				// ... add private dependencies that you statically link with here ...	
				"LuauLibrary"
			}
			);


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]{
                "UnrealEd",
                "Projects"
            });
        }
    }
}
