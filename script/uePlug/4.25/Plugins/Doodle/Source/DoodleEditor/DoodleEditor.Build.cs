// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
//using System.IO;

//这个模块只是用来创建不同时间段加载用的, 所以只用依赖这一个
public class doodleEditor : ModuleRules
{
    public doodleEditor(ReadOnlyTargetRules Target) : base(Target)
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
                "UnrealEd",

                "Engine",//几何缓存的骨骼依赖
                "UnrealEd", //创建actor工厂类需要
                // ... add other public dependencies that you statically link with here ...
                //abc的包装需要
                "AlembicLib",
                "MeshUtilities",
                "MaterialUtilities",
                "PropertyEditor",
                "SlateCore",
                "Slate",
                "EditorStyle",
                "Eigen",
                "RenderCore",
                "RHI"

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
                "doodle",



                "ContentBrowser",
                "EditorScriptingUtilities",
                "GeometryCache",  //复制材质依赖
                "AssetTools",     //资产工具 导入依赖

                "DesktopPlatform",//桌面依赖  打开文件对话框的依赖
				"RenderCore",
				////abc批量导入需要
                //"AlembicLibrary",
				"AIModule",//这个是ai控制使用的
                "PlacementMode",//这个是创建面板用的
                "AssetRegistry"//资源注册表模块
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
