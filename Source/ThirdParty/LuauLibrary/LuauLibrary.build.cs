using UnrealBuildTool;

public class LuauLibrary : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public LuauLibrary(ReadOnlyTargetRules Target) : base(Target)
#else
	public LuauLibrary(TargetInfo Target)
#endif
	{
		Type = ModuleType.External;

        string ThirdPartyDirectory = System.IO.Path.Combine(ModuleDirectory, "Binaries");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "isocline.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.Analysis.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.Ast.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.CLI.lib.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.CodeGen.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.Compiler.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.Config.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.EqSat.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Win", "x64", "Luau.VM.lib"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Mac", "x64", "liblua53_mac.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libisocline.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.Analysis.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.Ast.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.CLI.lib.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.CodeGen.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.Compiler.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.Config.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.EqSat.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "x64", "libLuau.VM.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.LinuxArm64)
        {
            //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Linux", "ARM64", "liblua53_linux_aarch64.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Android", "ARMv7", "liblua53_android.a"));
            //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "Android", "ARM64", "liblua53_android64.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyDirectory, "IOS", "ARM64", "liblua53_ios.a"));
        }
	}
}
