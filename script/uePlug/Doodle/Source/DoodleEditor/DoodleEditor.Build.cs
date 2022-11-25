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
                "RHI",
                "DeveloperSettings",// 显示设置模块
                "EditorSubsystem",//子系统依赖

                // 资产自定义模块
                // "ContentBrowserAssetDataSource"

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
                "AlembicLibrary",//abc批量导入需要
				"AIModule",//这个是ai控制使用的
                "PlacementMode",//这个是创建面板用的
                "AssetRegistry",//资源注册表模块
                "JsonUtilities",// 批量导入需要读取json
                "Json",// 批量导入需要读取json
                "SourceControl",//源代码管理,

                "LevelSequence",//创建定序器
                "Sequencer",//修改定序器
                "MovieScene",//更新定序器场景
                "CinematicCamera",// 相机模块
                "MovieSceneTools",// /更新定序器场景
                "DesktopWidgets",// 打开文件对话框小步件
                "FBX", // fbx读取需要
                "ImageCore",// 调整图像使用
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
