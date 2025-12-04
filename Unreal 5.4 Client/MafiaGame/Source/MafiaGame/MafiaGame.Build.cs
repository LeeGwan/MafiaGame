// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MafiaGame : ModuleRules
{
	public MafiaGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		bEnableExceptions = true;
		bUseRTTI = true;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"NavigationSystem",
			"AIModule",
			"UMG",
			"Niagara",
			"EnhancedInput",
			"Sockets",
			"Networking",

		
			"CoreOnline",            
			"OnlineSubsystem",       
			"OnlineSubsystemUtils"  
		});
		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
