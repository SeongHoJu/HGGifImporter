// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class HGGifImporter : ModuleRules
	{
		public HGGifImporter(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"HGGifImporter/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
                    "CoreUObject",
                    "Engine",
                    "UnrealEd",
                    "Paper2D",
                    "Paper2DEditor",
                    "ContentBrowser",
                    "AssetTools",
                    "Projects"
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					
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
}