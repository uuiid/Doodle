// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
//using System.IO;

public class doodle : ModuleRules
{
	public doodle(ReadOnlyTargetRules Target) : base(Target)
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

                "ContentBrowser",
                "EditorScriptingUtilities",
				//我的abc导入
				"GeometryCache",
                "AlembicLibrary"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

                "ContentBrowser",
                "EditorScriptingUtilities",
				"GeometryCache",  //复制材质依赖
				"AssetTools",     //资产工具 导入依赖

				"DesktopPlatform",//桌面依赖  打开文件对话框的依赖
				"AlembicImporter",//abc导入依赖
				"AlembicLibrary",
				"RenderCore",

				// ... add private dependencies that you statically link with here ...	
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
