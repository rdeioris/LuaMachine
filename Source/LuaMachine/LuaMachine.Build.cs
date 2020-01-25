// Copyright 2019 - Roberto De Ioris

using UnrealBuildTool;

public class LuaMachine : ModuleRules
{
    public LuaMachine(ReadOnlyTargetRules Target) : base(Target)
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
                "Slate",
                "SlateCore",
                "UMG",
                "InputCore",
                "HTTP",
                "Json"
				// ... add private dependencies that you statically link with here ...	
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

        string ThirdPartyDirectory = System.IO.Path.Combine(ModuleDirectory, "..", "ThirdParty");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "lua53_win64.lib"));
        }

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "lua53_mac.a"));
        }

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "lua53_linux64.a"));
        }

        if (Target.Platform == UnrealTargetPlatform.LinuxAArch64)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "lua53_linux_aarch64.a"));
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "liblua53_android.a"));
        }

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "lua53_ios.a"));
        }

    }
}
