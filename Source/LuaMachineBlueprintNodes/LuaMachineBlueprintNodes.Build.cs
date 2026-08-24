// Copyright 2018-2025 - Roberto De Ioris

using UnrealBuildTool;

public class LuaMachineBlueprintNodes : ModuleRules
{
    public LuaMachineBlueprintNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	    	"CoreUObject",
                "Engine",
                "LuaMachine",
                "BlueprintGraph",
                "SlateCore",
                "Slate",
                "GraphEditor",
                "UnrealEd",
                "KismetCompiler"
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
