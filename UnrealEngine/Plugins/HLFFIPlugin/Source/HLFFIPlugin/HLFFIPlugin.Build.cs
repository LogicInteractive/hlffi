// Copyright LogicInteractive. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class HLFFIPlugin : ModuleRules
{
	public HLFFIPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../../ThirdParty/hlffi"),
				Path.Combine(ModuleDirectory, "../../ThirdParty/hashlink/include"),
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// HashLink and HLFFI library linking
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
		string HashLinkLibPath = Path.Combine(ThirdPartyPath, "hashlink/lib/Win64");
		string HLFFILibPath = Path.Combine(ThirdPartyPath, "hlffi/lib/Win64");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Link against libhl.lib
			PublicAdditionalLibraries.Add(Path.Combine(HashLinkLibPath, "libhl.lib"));

			// Link against hlffi.lib
			PublicAdditionalLibraries.Add(Path.Combine(HLFFILibPath, "hlffi.lib"));

			// Copy DLL to binaries folder
			string DLLPath = Path.Combine(HashLinkLibPath, "libhl.dll");
			if (File.Exists(DLLPath))
			{
				RuntimeDependencies.Add(DLLPath);
				PublicDelayLoadDLLs.Add("libhl.dll");
			}
		}

		// Define HLFFI_IMPLEMENTATION in one compilation unit only
		PublicDefinitions.Add("HLFFI_EXT_UNREAL=1");

		// Disable some warnings that HashLink headers may trigger
		bEnableUndefinedIdentifierWarnings = false;
	}
}
